/**
 * @file persistence.hpp
 * @brief SQLite persistence layer for sensor readings
 */
#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <cstdint>
#include <string>

struct sqlite3;

/**
 * @class Database
 * @brief Thin wrapper around an SQLite3 database for storing readings
 */
class Database {
public:
  /**
   * @brief Open (or create) the database and ensure the readings table exists
   * @param dbPath Path to the SQLite database file
   */
  explicit Database(const std::string &dbPath);

  /// @brief Close the database handle
  ~Database();

  Database(const Database &) = delete;
  Database &operator=(const Database &) = delete;

  /**
   * @brief Insert a single reading row
   * @param ts          Unix timestamp (seconds since epoch)
   * @param rawAdc      Raw ADC value
   * @param moisturePct Calibrated moisture percentage
   * @param needsWater  Whether the plant needs water
   */
  void insertReading(std::int64_t ts, int rawAdc, double moisturePct,
                     bool needsWater);

private:
  sqlite3 *db_ = nullptr;
};

#endif
