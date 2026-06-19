/**
 * @file console_ui.hpp
 * @brief Console frontend — prints formatted status lines to stdout
 */
#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include <cstdint>

namespace console_ui {

/// @brief Print the column-header line
void printHeader();

/**
 * @brief Print a single reading as a formatted status line
 * @param ts          Unix timestamp (used to derive HH:MM:SS)
 * @param rawAdc      Raw ADC value from the sensor
 * @param moisturePct Calibrated moisture percentage
 * @param needsWater  Whether the plant needs watering
 */
void printReading(std::int64_t ts, int rawAdc, double moisturePct,
                  bool needsWater);

} // namespace console_ui

#endif // CONSOLE_UI_H
