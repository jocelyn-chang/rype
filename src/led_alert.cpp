/**
 * @file led_alert.cpp
 * @brief Implementation of the LedAlert class
 */
#include "led_alert.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#if defined(__linux__)
#include <cstdio>
#include <fcntl.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#if defined(__linux__)
#endif

#if defined(__linux__)
namespace {

std::string errnoMessage() {
  return "(errno=" + std::to_string(errno) + ": " +
         std::strerror(errno) + ")";
}

} // namespace
#endif

LedAlert::LedAlert(int gpioPin) : gpioPin_(gpioPin) {
#if defined(__linux__)
  const char *chipPath = "/dev/gpiochip0";

  int chipFd = ::open(chipPath, O_RDONLY | O_CLOEXEC);
  if (chipFd < 0) {
    std::cerr << "[warn] LED init failed for GPIO " << gpioPin_
              << ": cannot open " << chipPath << " "
              << errnoMessage() << "\n";
    return;
  }

  gpiohandle_request request {};
  request.lineoffsets[0] = static_cast<unsigned int>(gpioPin_);
  request.flags = GPIOHANDLE_REQUEST_OUTPUT;
  request.default_values[0] = 0;
  request.lines = 1;
  std::snprintf(request.consumer_label, sizeof(request.consumer_label),
                "plant_monitor");

  if (::ioctl(chipFd, GPIO_GET_LINEHANDLE_IOCTL, &request) < 0) {
    std::cerr << "[warn] LED init failed for GPIO " << gpioPin_
              << ": GPIO_GET_LINEHANDLE_IOCTL failed "
              << errnoMessage() << "\n";
    ::close(chipFd);
    return;
  }

  ::close(chipFd);
  lineHandleFd_ = request.fd;
  ready_ = true;
#else
  std::cerr << "[warn] LED alerts are only supported on Linux GPIO systems.\n";
#endif
}

LedAlert::~LedAlert() {
  try {
#if defined(__linux__)
    if (lineHandleFd_ >= 0) {
      set(false);
      ::close(lineHandleFd_);
      lineHandleFd_ = -1;
    }
#endif
  } catch (...) {
    // Best-effort cleanup — never throw from a destructor
  }
}

void LedAlert::set(bool on) {
#if defined(__linux__)
  if (!ready_ || lineHandleFd_ < 0) return;

  gpiohandle_data values {};
  values.values[0] = static_cast<unsigned char>(on ? 1 : 0);

  if (::ioctl(lineHandleFd_, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &values) < 0) {
    ready_ = false;
    std::cerr << "[warn] LED write failed for GPIO " << gpioPin_
              << ": " << errnoMessage() << "\n";
  }
#else
  (void)on;
#endif
}
