/**
 * @file discord_notify.cpp
 * @brief Implementation of the DiscordNotifier class using libcurl.
 *
 * When FEATURE_DISCORD_NOTIFICATIONS is 0 (the default), this file provides
 * no-op stubs so it compiles without libcurl. Setting the flag to 1 in
 * feature_flags.hpp to enable the real implementation.
 */
#include "discord_notify.hpp"
#include "feature_flags.hpp"

#if FEATURE_DISCORD_NOTIFICATIONS
// ---- Full implementation (libcurl) ----------------------------------------

#include <curl/curl.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

static size_t discardResponse(char *, size_t size, size_t nmemb, void *) {
  return size * nmemb;
}

DiscordNotifier::DiscordNotifier(std::string webhookUrl, std::string plantName)
    : webhookUrl_(std::move(webhookUrl)), plantName_(std::move(plantName)) {}

void DiscordNotifier::sendAlert(double moisturePct) {
  std::ostringstream json;
  json << "{\"content\": \"" << plantName_ << " needs water!  "
       << "Soil moisture is currently " << std::fixed << std::setprecision(1)
       << moisturePct << "% - please water your plant soon."
       << "\"}";
  post(json.str());
}

void DiscordNotifier::sendRecovery(double moisturePct) {
  std::ostringstream json;
  json << "{\"content\": \"" << plantName_ << " soil is now healthy.  "
       << "Moisture level: " << std::fixed << std::setprecision(1)
       << moisturePct << "%."
       << "\"}";
  post(json.str());
}

void DiscordNotifier::post(const std::string &jsonPayload) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    throw std::runtime_error("DiscordNotifier: curl_easy_init() failed");
  }
  curl_easy_setopt(curl, CURLOPT_URL, webhookUrl_.c_str());
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discardResponse);
  CURLcode res = curl_easy_perform(curl);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    throw std::runtime_error(std::string("DiscordNotifier: curl error - ") +
                             curl_easy_strerror(res));
  }
}

#else
// ---- Stub implementation (feature disabled) --------------------------------

DiscordNotifier::DiscordNotifier(std::string webhookUrl, std::string plantName)
    : webhookUrl_(std::move(webhookUrl)), plantName_(std::move(plantName)) {}

void DiscordNotifier::sendAlert(double /*moisturePct*/) {}
void DiscordNotifier::sendRecovery(double /*moisturePct*/) {}
void DiscordNotifier::post(const std::string & /*jsonPayload*/) {}

#endif // FEATURE_DISCORD_NOTIFICATIONS