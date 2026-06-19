/**
 * @file status_web.hpp
 * @brief Declaration of the StatusWeb class
 */

#ifndef STATUS_WEB_HPP
#define STATUS_WEB_HPP

#include <mutex>

/**
 * @class StatusWeb
 * @brief Provides a local web interface for displaying current plant soil status
 * 
 * This class handles displaying the current soil status through a local web page.
 * The monitoring loop updates the moisture level and plant status, and the web server displays the values.
 */
class StatusWeb {

public:
  /**
   * @brief Update the current moisture and watering status
   * @param moisturePct Current moisture percentage
   * @param needsWater Whether the plant currently needs water
   */
  void update(double moisturePct, bool needsWater);

  /**
   * @brief Start the web server on the specified port
   * @param port TCP port number for the web server (default: 8080)
   */
  void start(int port = 8080);

private:
  double moisturePct_ = 0.0;  ///< Current moisture percentage
  bool needsWater_ = false;   ///< Whether the plant needs water
  std::mutex mutex_;          ///< Mutex for thread-safe access to shared data
};

#endif
