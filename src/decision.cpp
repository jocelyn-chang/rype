/**
 * @file decision.cpp
 * @brief Implementation of the WaterDecider class
 */
#include "decision.hpp"

WaterDecider::WaterDecider(double threshold, int consecutiveCount)
    : threshold_(threshold), requiredCount_(consecutiveCount) {}

bool WaterDecider::update(double moisturePct) {
  if (moisturePct < threshold_) {
    ++dryStreak_;
  } else {
    dryStreak_ = 0;
  }
  return dryStreak_ >= requiredCount_;
}
