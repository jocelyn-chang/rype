/**
 * @file weather.hpp
 * @brief Simple Open-Meteo weather lookup helpers.
 */
#pragma once

#include <optional>

namespace weather {

/**
 * @brief Fetch maximum precipitation probability for the next 12 hours.
 *
 * @param latitude Latitude used for Open-Meteo forecast request.
 * @param longitude Longitude used for Open-Meteo forecast request.
 * @return Maximum precipitation probability [0,100] if successful;
 *         std::nullopt if the request or parse fails.
 */
std::optional<double> fetchMaxPrecipProbabilityNext12h(double latitude,
                                                       double longitude);

} // namespace weather
