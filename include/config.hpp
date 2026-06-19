/**
 * @file config.hpp
 * @brief Compile-time constants for the plant monitoring system
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace config {

/// @brief Path to the Arduino serial device
inline constexpr const char *SERIAL_PORT = "/tmp/plant_serial_read";

/// @brief Baud rate for Arduino serial communication
inline constexpr int BAUD_RATE = 9600;

/// @brief ADC value when the sensor is in open air
inline constexpr int DRY_VALUE = 590;

/// @brief ADC value when the sensor is fully submerged in water
inline constexpr int WET_VALUE = 273;

/// @brief Moisture percentage below which soil is considered "dry"
inline constexpr double MOISTURE_THRESHOLD = 66.2;

/// @brief Open-Meteo latitude used for outdoor rain checks
inline constexpr double WEATHER_LATITUDE = 43.6532;

/// @brief Open-Meteo longitude used for outdoor rain checks
inline constexpr double WEATHER_LONGITUDE = -79.3832;

/// @brief If max rain probability exceeds this, suppress watering for outdoor plants
inline constexpr double RAIN_SUPPRESSION_THRESHOLD = 70.0;

/// @brief Consecutive dry samples required to flag "needs water"
inline constexpr int CONSECUTIVE_DRY_COUNT = 1;

/// @brief Number of samples in the moving-average filter window
inline constexpr int FILTER_WINDOW_SIZE = 5;

/// @brief SQLite database file path (relative to working directory)
inline constexpr const char *DB_PATH = "plant_monitor.db";

/// @brief Number of seconds to wait between consecutive sensor samples
inline constexpr int SAMPLING_INTERVAL_S = 10;

/// @brief Camera index used by the terminal Vision health check
inline constexpr int VISION_CAMERA_INDEX = 0;

/// @brief BCM-numbered GPIO pin connected to the alert LED (physical pin 7 = BCM4)
inline constexpr int LED_GPIO_PIN = 4;

/// @brief Full Discord Incoming Webhook URL used by DiscordNotifier
inline constexpr const char *DISCORD_WEBHOOK_URL =
    "https://discord.com/api/webhooks/1481051918376898786/eE61WAY2yKWB71Vv0UhFTVwkU-5EeE1iDKKyaT6cYzfFxjjTRowt52e5Rh0WBXgXksXB";

/// @brief Human-readable plant name included in every Discord notification
inline constexpr const char *PLANT_NAME = "My Plant";

} // namespace config

#endif
