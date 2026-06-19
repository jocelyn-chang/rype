/**
 * @file calibration.cpp
 * @brief Calibration implementation — linear mapping with clamping
 */
#include "calibration.hpp"
#include <algorithm>

double rawToMoisturePct(int raw, int dryValue, int wetValue) {
  if (dryValue == wetValue)
    return 0.0;

  double pct = static_cast<double>(raw - dryValue) /
               static_cast<double>(wetValue - dryValue) * 100.0;

  return std::clamp(pct, 0.0, 100.0);
}
