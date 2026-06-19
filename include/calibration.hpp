/**
 * @file calibration.hpp
 * @brief Converts raw ADC values to a soil moisture percentage
 */
#ifndef CALIBRATION_H
#define CALIBRATION_H

/**
 * @brief Convert a raw ADC reading to a moisture percentage
 * @param raw       The raw ADC reading (0–1023 for 10-bit)
 * @param dryValue  ADC value when sensor is completely dry
 * @param wetValue  ADC value when sensor is fully submerged
 * @return Moisture percentage in [0.0, 100.0]
 */
double rawToMoisturePct(int raw, int dryValue, int wetValue);

#endif
