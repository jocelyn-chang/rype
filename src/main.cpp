/**
 * @file main.cpp
 * @brief Entry point and sampling loop for the Rype plant monitoring system.
 *
 * @details
 * Startup sequence:
 *  1. Show animated splash screen.
 *  2. User selects their plant from the menu (sets moisture threshold).
 *  3. Live monitoring table is displayed with colour-coded readings.
 *  4. A background thread listens for keybind input:
 *       [I] - plant info panel
 *       [V] - run computer-vision health check
 *       [H] - help screen
 *       [Q] - graceful shutdown
 *
 * Optional features (Discord, LED, web dashboard) are guarded by the
 * flags defined in feature_flags.hpp. When a flag is false the related
 * code is compiled away entirely.
 *
 * @author Rype Team 1
 */

#include "calibration.hpp"
#include "config.hpp"
#include "decision.hpp"
#include "feature_flags.hpp"
#include "filter.hpp"
#include "persistence.hpp"
#include "sensor_reader.hpp"
#include "terminal_ui.hpp"

// --- Conditionally included headers for optional features -----------------
#include "discord_notify.hpp"
#include "led_alert.hpp"
#include "status_web.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

/// Set to false by SIGINT or by pressing [Q] to trigger graceful shutdown.
static volatile bool g_running = true;

/// Latest moisture reading shared between the sensor loop and the keybind thread.
static std::atomic<double> g_latestMoisture{0.0};

enum class CvHealthState : int {
  Unknown = 0,
  Running = 1,
  Healthy = 2,
  Unhealthy = 3,
  Error = 4,
};

/// Latest computer-vision health status shared between threads.
static std::atomic<int> g_cvHealthState{static_cast<int>(CvHealthState::Unknown)};

/**
 * @brief SIGINT handler — request a graceful shutdown.
 * @param sig Signal number (unused).
 */
static void signalHandler(int /*sig*/) { g_running = false; }

static std::string speciesForPlant(const PlantProfile &plant) {
  if (plant.name.find("Aloe") != std::string::npos ||
      plant.name.find("aloe") != std::string::npos) {
    return "aloe";
  }
  return "pothos";
}

static std::string cvHealthLabel(CvHealthState state) {
  switch (state) {
    case CvHealthState::Healthy:
      return "HEALTHY";
    case CvHealthState::Unhealthy:
      return "UNHEALTHY";
    case CvHealthState::Running:
      return "RUNNING";
    case CvHealthState::Error:
      return "ERROR";
    case CvHealthState::Unknown:
    default:
      return "UNKNOWN";
  }
}

static CvHealthState runVisionHealthCheck(const PlantProfile &plant) {
  const std::string species = speciesForPlant(plant);
  const std::string cmd =
      "python3 vision/plant_matcher.py predict-health-camera --species " +
  species + " --camera " + std::to_string(config::VISION_CAMERA_INDEX) +
  " --json 2>/dev/null";

  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    return CvHealthState::Error;
  }

  std::string output;
  char buf[512];
  while (fgets(buf, sizeof(buf), pipe) != nullptr) {
    output += buf;
  }

  const int rc = pclose(pipe);
  if (rc != 0) {
    return CvHealthState::Error;
  }

  if (output.find("\"health\": \"unhealthy\"") != std::string::npos) {
    return CvHealthState::Unhealthy;
  }
  if (output.find("\"health\": \"healthy\"") != std::string::npos) {
    return CvHealthState::Healthy;
  }
  return CvHealthState::Error;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

/**
 * @brief Application entry point.
 *
 * @details
 * Constructs all subsystems, runs the startup UI sequence, then enters the
 * main sensor-read → filter → decide → alert → persist loop.
 *
 * @return 0 on clean exit.
 */
int main() {
  std::signal(SIGINT, signalHandler);

  // -------------------------------------------------------------------------
  // 1. Terminal UI startup — splash and plant selection
  // -------------------------------------------------------------------------

  TerminalUI   ui;
  ui.showSplash();
  PlantProfile plant = ui.selectPlant();

  // -------------------------------------------------------------------------
  // 2. Core subsystem construction
  // -------------------------------------------------------------------------

  SensorReader        sensor(config::SERIAL_PORT, config::BAUD_RATE);
  MovingAverageFilter filter(config::FILTER_WINDOW_SIZE);

  // Use the plant's minimum ideal moisture as the dry threshold when it is
  // lower than the global config value, so each plant gets a tailored alert.
  double threshold = (plant.minMoisture < config::MOISTURE_THRESHOLD)
                         ? plant.minMoisture
                         : config::MOISTURE_THRESHOLD;

  WaterDecider decider(threshold, config::CONSECUTIVE_DRY_COUNT);
  Database     db(config::DB_PATH);

  // -------------------------------------------------------------------------
  // 3. Optional feature construction (guarded by feature flags)
  // -------------------------------------------------------------------------

  /// Tracks the previous needsWater state to avoid repeated Discord messages.
  bool lastNeedsWater = true;

  // Web dashboard
  std::unique_ptr<StatusWeb>       web;
  std::thread                      webThread;

  if constexpr (feature_flags::ENABLE_STATUS_WEB) {
    web = std::make_unique<StatusWeb>();
    webThread = std::thread([&web]() { web->start(8080); });
  }

  // LED alert
  std::unique_ptr<LedAlert> led;
  if constexpr (feature_flags::ENABLE_LED_ALERTS) {
    led = std::make_unique<LedAlert>(config::LED_GPIO_PIN);
    if constexpr (feature_flags::ENABLE_LED_SOLID_ON_TEST) {
      led->set(true);
    }
  }

  // Discord notifier
  std::unique_ptr<DiscordNotifier> discord;
  if constexpr (feature_flags::ENABLE_DISCORD_NOTIFICATIONS) {
    discord = std::make_unique<DiscordNotifier>(
        config::DISCORD_WEBHOOK_URL, plant.name);
  }

  // -------------------------------------------------------------------------
  // 4. Keybind input thread
  // -------------------------------------------------------------------------

  /**
   * @brief Background thread that handles keyboard input during monitoring.
   *
   * @details
   * Reads single keypresses without requiring Enter. When a recognised key
   * is pressed it renders the appropriate overlay, then redraws the
   * monitoring header so the table continues cleanly.
   */
  std::thread inputThread([&ui, &plant]() {
    while (g_running) {
      char key = ui.getKey();
      if (key == 'i') {
        const auto state = static_cast<CvHealthState>(g_cvHealthState.load());
        ui.showInfoPanel(plant, g_latestMoisture.load(), cvHealthLabel(state));
        ui.printMonitorHeader(plant);
      } else if (key == 'v') {
        g_cvHealthState.store(static_cast<int>(CvHealthState::Running));
        const CvHealthState state = runVisionHealthCheck(plant);
        g_cvHealthState.store(static_cast<int>(state));
        ui.printMonitorHeader(plant);
      } else if (key == 'h') {
        ui.showHelpScreen();
        ui.printMonitorHeader(plant);
      } else if (key == 'q') {
        g_running = false;
      }
    }
  });

  // -------------------------------------------------------------------------
  // 5. Draw the initial monitoring header
  // -------------------------------------------------------------------------

  ui.printMonitorHeader(plant);

  // Pre-compute the first wake-up time for the interval sleep.
  auto nextWakeTime = std::chrono::steady_clock::now();

  // =========================================================================
  // 6. Main sampling loop
  // =========================================================================
  while (g_running) {

    // Advance the next wake-up time by one sampling interval.
    nextWakeTime += std::chrono::seconds(config::SAMPLING_INTERVAL_S);

    try {
      // Step 1 — Read a raw line from the sensor over serial.
      std::string line   = sensor.readLine();
      int         rawAdc = SensorReader::parseRaw(line);

      if (rawAdc < 0) {
        // Malformed line — log and wait for the next interval.
        std::cerr << "  [warn] Bad reading: \"" << line << "\" - skipping.\n";

      } else {

        // Step 2 — Filter and convert to a calibrated moisture percentage.
        filter.addSample(rawAdc);
        double filteredRaw = filter.getValue();
        double moisturePct = rawToMoisturePct(
            static_cast<int>(filteredRaw),
            config::DRY_VALUE,
            config::WET_VALUE);

        // Share the latest value with the keybind thread (for the info panel).
        g_latestMoisture.store(moisturePct);

        // Step 3 — Compute ideal-range status for UI and LED behavior.
        bool tooWetNow = moisturePct > plant.maxMoisture;
        bool needsWaterNow = moisturePct < plant.minMoisture;
        bool outOfIdealRange = needsWaterNow || tooWetNow;

        // Keep debounced dry-state logic for persistence/notifications.
        bool needsWater = decider.update(moisturePct);

        // Step 4 — LED: on whenever moisture is outside the ideal range.
        if constexpr (feature_flags::ENABLE_LED_ALERTS) {
          if (led) led->set(outOfIdealRange);
        }

        // Step 5 — Discord: notify only on state transitions.
        if constexpr (feature_flags::ENABLE_DISCORD_NOTIFICATIONS) {
          if (discord) {
            if (needsWater && !lastNeedsWater) {
              try { discord->sendAlert(moisturePct); }
              catch (const std::exception &ex) {
                std::cerr << "  [warn] Discord alert failed: "
                          << ex.what() << "\n";
              }
            } else if (!needsWater && lastNeedsWater) {
              try { discord->sendRecovery(moisturePct); }
              catch (const std::exception &ex) {
                std::cerr << "  [warn] Discord recovery failed: "
                          << ex.what() << "\n";
              }
            }
          }
        }
        lastNeedsWater = needsWater;

        // Step 6 — Persist the reading to SQLite.
        auto now = std::chrono::system_clock::now();
        auto ts  = std::chrono::system_clock::to_time_t(now);
        db.insertReading(static_cast<std::int64_t>(ts), rawAdc,
                         moisturePct, needsWater);

        // Step 7 — Render the reading row in the terminal table.
        const auto cvState = static_cast<CvHealthState>(g_cvHealthState.load());
        ui.printReading(static_cast<std::int64_t>(ts), rawAdc,
            moisturePct, needsWaterNow, tooWetNow,
            cvHealthLabel(cvState));

        // Step 8 — Push to the web dashboard.
        if constexpr (feature_flags::ENABLE_STATUS_WEB) {
          if (web) web->update(moisturePct, needsWater);
        }

      } // end else (valid rawAdc)

    } catch (const std::exception &ex) {
      std::cerr << "  [error] " << ex.what() << "\n";
    }

    // Step 9 — Sleep in 100 ms ticks until the next interval so that
    //           SIGINT and [Q] are handled promptly.
    while (g_running &&
           std::chrono::steady_clock::now() < nextWakeTime) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

  }
  // =========================================================================

  std::cout << "\n  Shutting down Rype. Goodbye!\n\n";

  inputThread.detach();

  if constexpr (feature_flags::ENABLE_STATUS_WEB) {
    if (webThread.joinable()) webThread.detach();
  }

  return 0;
}