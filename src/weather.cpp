/**
 * @file weather.cpp
 * @brief Open-Meteo integration for precipitation probability checks.
 */

#include "weather.hpp"

#include <algorithm>
#include <cctype>
#include <curl/curl.h>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

size_t writeCallback(char *contents, size_t size, size_t nmemb, void *userp) {
  const size_t totalSize = size * nmemb;
  auto *buffer = static_cast<std::string *>(userp);
  buffer->append(contents, totalSize);
  return totalSize;
}

std::optional<double> parseMaxPrecipProbability(const std::string &json) {
  const std::string key = "\"precipitation_probability\"";
  const size_t keyPos = json.find(key);
  if (keyPos == std::string::npos) return std::nullopt;

  const size_t arrayStart = json.find('[', keyPos);
  const size_t arrayEnd = json.find(']', arrayStart);
  if (arrayStart == std::string::npos || arrayEnd == std::string::npos ||
      arrayEnd <= arrayStart) {
    return std::nullopt;
  }

  const std::string arrayBody = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
  std::stringstream ss(arrayBody);
  std::string token;
  bool foundAny = false;
  double maxProb = 0.0;

  while (std::getline(ss, token, ',')) {
    token.erase(std::remove_if(token.begin(), token.end(),
                               [](unsigned char c) { return std::isspace(c); }),
                token.end());
    if (token.empty() || token == "null") continue;

    try {
      const double value = std::stod(token);
      if (!foundAny || value > maxProb) {
        maxProb = value;
        foundAny = true;
      }
    } catch (...) {
      // Ignore malformed values and continue parsing the array.
    }
  }

  if (!foundAny) return std::nullopt;
  return maxProb;
}

} // namespace

namespace weather {

std::optional<double> fetchMaxPrecipProbabilityNext12h(double latitude,
                                                       double longitude) {
  CURL *curl = curl_easy_init();
  if (!curl) return std::nullopt;

  std::ostringstream url;
  url << "https://api.open-meteo.com/v1/forecast"
      << "?latitude=" << latitude
      << "&longitude=" << longitude
      << "&hourly=precipitation_probability"
      << "&forecast_hours=12"
      << "&timezone=auto";

  std::string responseBody;

  curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "plant-monitor/1.0");

  const CURLcode rc = curl_easy_perform(curl);
  long httpCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
  curl_easy_cleanup(curl);

  if (rc != CURLE_OK || httpCode != 200) return std::nullopt;
  return parseMaxPrecipProbability(responseBody);
}

} // namespace weather
