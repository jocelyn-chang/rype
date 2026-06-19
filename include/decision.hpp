/**
 * @file decision.hpp
 * @brief Determines whether the plant needs watering
 */
#ifndef DECISION_H
#define DECISION_H

/**
 * @class WaterDecider
 * @brief Tracks consecutive dry readings and decides when to flag
 */
class WaterDecider {
public:
  /**
   * @brief Construct a WaterDecider
   * @param threshold        Moisture percentage below which soil is "dry"
   * @param consecutiveCount Consecutive dry samples required to trigger the
   * flag
   */
  WaterDecider(double threshold, int consecutiveCount);

  /**
   * @brief Feed a new moisture percentage into the decider
   * @param moisturePct Current moisture percentage [0–100]
   * @return true if the plant currently needs water
   */
  bool update(double moisturePct);

private:
  double threshold_;
  int requiredCount_;
  int dryStreak_ = 0; ///< Running count of consecutive dry readings
};

#endif
