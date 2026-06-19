/**
 * @file discord_notify.hpp
 * @brief Declaration of the DiscordNotifier class.
 */

#pragma once
#include <string>

/**
 * @class DiscordNotifier
 * @brief Sends notifications to a Discord channel via webhook when plant needs water
 */
class DiscordNotifier {
public:
  /**
   * @brief Construct a Discord notifier with the given webhook and plant name
   * @param webhookUrl Full Discord webhook URL for posting messages
   * @param plantName Human-readable name of the plant for notifications
   */
  DiscordNotifier(std::string webhookUrl, std::string plantName);
  
  /**
   * @brief Send an alert notification indicating the plant needs water
   * @param moisturePct Current moisture percentage to include in the message
   */
  void sendAlert(double moisturePct);
  
  /**
   * @brief Send a recovery notification indicating the plant has been watered
   * @param moisturePct Current moisture percentage to include in the message
   */
  void sendRecovery(double moisturePct);

private:
  std::string webhookUrl_;  ///< Discord webhook URL for posting messages
  std::string plantName_;   ///< Name of the plant for notifications
  
  /**
   * @brief Post a JSON payload to the Discord webhook
   * @param jsonPayload JSON string containing the Discord message
   */
  void post(const std::string &jsonPayload);
};