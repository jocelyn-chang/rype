/**
 * @file console_ui.cpp
 * @brief Console frontend implementation
 */
#include "console_ui.hpp"
#include <ctime>
#include <iomanip>
#include <iostream>

namespace console_ui {

void printHeader() {
  std::cout << std::left << std::setw(10) << "Time"
            << "| " << std::setw(10) << "Raw ADC"
            << "| " << std::setw(14) << "Moisture %"
            << "| "
            << "Status"
            << "\n";
  std::cout << std::string(50, '-') << "\n";
}

void printReading(std::int64_t ts, int rawAdc, double moisturePct,
                  bool needsWater) {
  std::time_t t = static_cast<std::time_t>(ts);
  std::tm *lt = std::localtime(&t);

  char timeBuf[16];
  std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", lt);

  std::cout << std::left << std::setw(10) << timeBuf << "| RAW " << std::setw(6)
            << rawAdc << "| " << std::setw(4) << static_cast<int>(moisturePct)
            << "%" << std::setw(9) << ""
            << "| " << (needsWater ? "NEEDS WATER" : "OK") << "\n";

  std::cout.flush();
}

} // namespace console_ui
