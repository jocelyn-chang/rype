/**
 * @file sensor_reader.cpp
 * @brief Implementation of the SensorReader class
 */
#include "sensor_reader.hpp"
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>

/// @brief Map integer baud rate to a termios speed constant
static speed_t baudToSpeed(int baud) {
  switch (baud) {
  case 9600:
    return B9600;
  case 19200:
    return B19200;
  case 38400:
    return B38400;
  case 57600:
    return B57600;
  case 115200:
    return B115200;
  default:
    return B9600;
  }
}

SensorReader::SensorReader(const std::string &port, int baud) {
  fd_ = ::open(port.c_str(), O_RDONLY | O_NOCTTY);
  if (fd_ < 0) {
    throw std::runtime_error("SensorReader: cannot open " + port + " — " +
                             std::strerror(errno));
  }

  struct termios tty {};
  if (tcgetattr(fd_, &tty) != 0) {
    ::close(fd_);
    throw std::runtime_error("SensorReader: tcgetattr failed");
  }

  cfsetispeed(&tty, baudToSpeed(baud));
  cfsetospeed(&tty, baudToSpeed(baud));

  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);
  tty.c_oflag &= ~OPOST;
  tty.c_cflag |= (CLOCAL | CREAD);

  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;

  tty.c_cc[VMIN] = 1;
  tty.c_cc[VTIME] = 0;

  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    ::close(fd_);
    throw std::runtime_error("SensorReader: tcsetattr failed");
  }
}

SensorReader::~SensorReader() {
  if (fd_ >= 0) {
    ::close(fd_);
  }
}

std::string SensorReader::readLine() {
  // Discard any stale data that arrived while we were sleeping,
  // so we always get the freshest reading from the sensor.
  tcflush(fd_, TCIFLUSH);

  std::string line;
  char c = 0;
  while (true) {
    ssize_t n = ::read(fd_, &c, 1);
    if (n < 0) {
      throw std::runtime_error("SensorReader: read error");
    }
    if (n == 0)
      continue;
    if (c == '\n')
      break;
    if (c != '\r')
      line += c;
  }
  return line;
}

int SensorReader::parseRaw(const std::string &line) {
  const std::string prefix = "RAW:";
  if (line.rfind(prefix, 0) != 0) {
    return -1;
  }
  try {
    return std::stoi(line.substr(prefix.size()));
  } catch (...) {
    return -1;
  }
}
