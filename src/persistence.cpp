/**
 * @file persistence.cpp
 * @brief Implementation of the Database class
 */
#include "persistence.hpp"
#include <sqlite3.h>
#include <stdexcept>

Database::Database(const std::string &dbPath) {
  int rc = sqlite3_open(dbPath.c_str(), &db_);
  if (rc != SQLITE_OK) {
    throw std::runtime_error(std::string("Database: cannot open ") + dbPath +
                             " — " + sqlite3_errmsg(db_));
  }

  const char *sql = "CREATE TABLE IF NOT EXISTS readings ("
                    "  ts           INTEGER NOT NULL,"
                    "  raw_adc      INTEGER NOT NULL,"
                    "  moisture_pct REAL    NOT NULL,"
                    "  needs_water  INTEGER NOT NULL"
                    ");";

  char *errMsg = nullptr;
  rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    std::string msg = errMsg ? errMsg : "unknown error";
    sqlite3_free(errMsg);
    throw std::runtime_error("Database: table creation failed — " + msg);
  }
}

Database::~Database() {
  if (db_) {
    sqlite3_close(db_);
  }
}

void Database::insertReading(std::int64_t ts, int rawAdc, double moisturePct,
                             bool needsWater) {
  const char *sql =
      "INSERT INTO readings (ts, raw_adc, moisture_pct, needs_water) "
      "VALUES (?, ?, ?, ?);";

  sqlite3_stmt *stmt = nullptr;
  int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    throw std::runtime_error(std::string("Database: prepare failed — ") +
                             sqlite3_errmsg(db_));
  }

  sqlite3_bind_int64(stmt, 1, ts);
  sqlite3_bind_int(stmt, 2, rawAdc);
  sqlite3_bind_double(stmt, 3, moisturePct);
  sqlite3_bind_int(stmt, 4, needsWater ? 1 : 0);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE) {
    throw std::runtime_error(std::string("Database: insert failed — ") +
                             sqlite3_errmsg(db_));
  }
}
