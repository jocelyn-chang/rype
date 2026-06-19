/**
 * @file led_alert.hpp
 * @brief Declaration of the LedAlert class.
 */

#pragma once
#include <string>

/**
 * @class LedAlert
 * @brief Controls a GPIO-connected LED for visual plant status alerts
 */
class LedAlert {
public:
  /**
   * @brief Construct an LED alert controller for the specified GPIO pin
   * @param gpioPin BCM-numbered GPIO pin connected to the LED (default: 4)
   */
  explicit LedAlert(int gpioPin = 4);
  
  /**
   * @brief Destructor to clean up GPIO resources
   */
  ~LedAlert();
  
  /**
   * @brief Turn the LED on or off
   * @param on true to turn LED on, false to turn it off
   */
  void set(bool on);

private:
  int gpioPin_;              ///< BCM-numbered GPIO pin
  int lineHandleFd_ = -1;    ///< Open GPIO line handle from gpiochip
  bool ready_ = false;       ///< True once GPIO setup succeeds
  
  /**
   * @brief Write a value to the active GPIO backend
   * @param path Full path to a sysfs file
   * @param value String value to write
   */
  static void sysfsWrite(const std::string &path, const std::string &value);
};