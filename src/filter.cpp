/**
 * @file filter.cpp
 * @brief Implementation of the MovingAverageFilter
 */
#include "filter.hpp"
#include <numeric>

MovingAverageFilter::MovingAverageFilter(int windowSize)
    : windowSize_(windowSize) {}

void MovingAverageFilter::addSample(int value) {
  samples_.push_back(value);
  if (static_cast<int>(samples_.size()) > windowSize_) {
    samples_.pop_front();
  }
}

double MovingAverageFilter::getValue() const {
  if (samples_.empty())
    return 0.0;
  double sum = std::accumulate(samples_.begin(), samples_.end(), 0.0);
  return sum / static_cast<double>(samples_.size());
}
