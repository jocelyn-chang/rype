/**
 * @file filter.hpp
 * @brief Simple moving-average filter for smoothing noisy ADC readings
 */
#ifndef FILTER_H
#define FILTER_H

#include <deque>

/**
 * @class MovingAverageFilter
 * @brief Sliding-window arithmetic-mean filter
 */
class MovingAverageFilter {
public:
  /**
   * @brief Construct a MovingAverageFilter
   * @param windowSize Number of samples to keep in the window
   */
  explicit MovingAverageFilter(int windowSize);

  /**
   * @brief Add a new raw ADC sample to the window
   * @param value The raw ADC sample value
   */
  void addSample(int value);

  /**
   * @brief Get the average of the current window contents
   * @return The moving average, or 0.0 if no samples have been added
   */
  double getValue() const;

private:
  int windowSize_;
  std::deque<int> samples_;
};

#endif
