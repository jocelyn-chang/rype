/**
 * @file sensor_reader.hpp
 * @brief Reads serial data from the Arduino over USB
 */
#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include <string>

/**
 * @class SensorReader
 * @brief Opens and reads from a serial port connected to the Arduino
 */
class SensorReader {
public:
  /**
   * @brief Construct a SensorReader for the given serial device
   * @param port Path to the serial device (e.g. "/dev/ttyACM0")
   * @param baud Baud rate (must match the Arduino sketch)
   */
  explicit SensorReader(const std::string &port, int baud);

  /// @brief Close the serial file descriptor
  ~SensorReader();

  SensorReader(const SensorReader &) = delete;
  SensorReader &operator=(const SensorReader &) = delete;

  /**
   * @brief Block until a full newline-terminated line is received
   * @return The line (without the trailing newline)
   */
  std::string readLine();

  /**
   * @brief Parse a "RAW:<value>" string and extract the integer value
   * @param line Input string, e.g. "RAW:523"
   * @return The parsed integer, or -1 on parse failure
   */
  static int parseRaw(const std::string &line);

private:
  int fd_ = -1; ///< File descriptor for the serial port
};

#endif
