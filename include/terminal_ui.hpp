/**
 * @file terminal_ui.hpp
 * @brief Declaration of the TerminalUI class and PlantProfile struct.
 *
 * @details
 * Provides a full-screen terminal interface for the plant monitoring system.
 * On startup the user is presented with:
 *   1. A welcome splash screen.
 *   2. A plant-selection menu (arrow keys or number keys).
 *   3. A live monitoring view with a keybind bar at the bottom.
 *
 * Keybinds available during monitoring:
 *   [I] - Show plant info panel
 *   [V] - Run computer-vision health check
 *   [H] - Show keybind help screen
 *   [Q] - Quit the application
 *
 * The class uses ANSI escape codes for colours and cursor control,
 * which work on macOS Terminal, Linux terminals, and the Raspberry Pi
 * console. Windows CMD is not supported.
 *
 * @author Rype Team 1
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

// ---------------------------------------------------------------------------
// PlantProfile
// ---------------------------------------------------------------------------

/**
 * @struct PlantProfile
 * @brief Holds species-specific information for a monitored plant.
 *
 * @details
 * Each profile stores the display name, ideal moisture range, watering
 * frequency tip, and a short fun fact shown in the info panel.
 */
struct PlantProfile {
  std::string name;             ///< Display name, e.g. "Pothos"
  std::string emoji;            ///< Single emoji used as a visual icon
  double      minMoisture;      ///< Lower bound of the ideal moisture range (%)
  double      maxMoisture;      ///< Upper bound of the ideal moisture range (%)
  std::string wateringTip;      ///< Short watering advice for this species
  std::string funFact;          ///< Fun fact shown in the info panel
};

// ---------------------------------------------------------------------------
// TerminalUI
// ---------------------------------------------------------------------------

/**
 * @class TerminalUI
 * @brief Manages all terminal rendering for the plant monitoring system.
 *
 * @details
 * Call order:
 *  1. showSplash()         — animated welcome screen (optional, ~1 s)
 *  2. selectPlant()        → returns the chosen PlantProfile
 *  3. printMonitorHeader() — draw the live-monitoring header
 *  4. printReading(...)    — call once per sensor cycle
 *  5. handleInput()        — call in a background thread to handle keybinds
 */
class TerminalUI {
public:

  /**
   * @brief Construct the UI and populate the built-in plant catalogue.
   */
  TerminalUI();

  /**
   * @brief Display an animated splash / welcome screen.
   *
   * @details
   * Clears the terminal and prints the Rype logo with a short animated
   * delay. Blocks for approximately 1.5 seconds total.
   */
  void showSplash() const;

  /**
   * @brief Show the plant-selection menu and block until the user picks one.
   *
   * @details
   * Renders a numbered list of available plant profiles. The user presses
   * a number key (1–N) to select. Arrow-key navigation is also supported.
   * The selected profile is returned so the caller can apply the correct
   * moisture thresholds.
   *
   * @return The PlantProfile chosen by the user.
   */
  PlantProfile selectPlant() const;

  /**
   * @brief Print the static header for the live monitoring table.
   *
   * @param plant The plant profile selected during setup, used to show
   *              the plant name and ideal moisture range in the header.
   */
  void printMonitorHeader(const PlantProfile &plant) const;

  /**
   * @brief Print one row of the live monitoring table.
   *
   * @param ts          Unix timestamp of the reading.
   * @param rawAdc      Raw ADC value from the sensor.
   * @param moisturePct Filtered moisture percentage (0–100).
  * @param needsWater  true when moisture is below the plant's ideal range.
  * @param tooWet      true when moisture is above the plant's ideal range.
  * @param cvHealth    Computer-vision health label (for example,
  *                    "HEALTHY" or "UNHEALTHY").
   */
  void printReading(std::int64_t ts, int rawAdc, double moisturePct,
             bool needsWater, bool tooWet,
             const std::string &cvHealth) const;

  /**
   * @brief Render the plant info panel overlay.
   *
   * @details
   * Triggered by pressing [I] during monitoring. Shows the plant name,
   * ideal moisture range, watering tip, and fun fact. Press any key
   * to dismiss.
   *
   * @param plant        The currently monitored plant profile.
   * @param moisturePct  Current moisture reading to display.
  * @param cvHealth     Current computer-vision health label.
   */
  void showInfoPanel(const PlantProfile &plant, double moisturePct,
              const std::string &cvHealth) const;

  /**
   * @brief Render the keybind help screen overlay.
   *
   * @details
   * Triggered by pressing [H] during monitoring. Lists all available
   * keybinds. Press any key to dismiss.
   */
  void showHelpScreen() const;

  /**
   * @brief Read a single keypress without requiring Enter.
   *
   * @details
   * Temporarily puts stdin into raw mode, reads one character, then
   * restores the original terminal settings. Used by the keybind handler
   * in the monitoring loop.
   *
   * @return The character pressed (lowercase).
   */
  char getKey() const;

  /**
   * @brief Return the full catalogue of supported plant profiles.
   *
   * @return Const reference to the internal plant profile vector.
   */
  const std::vector<PlantProfile> &plants() const { return plants_; }

private:

  /// Built-in catalogue of supported plant species.
  std::vector<PlantProfile> plants_;

  // ---- ANSI helpers --------------------------------------------------------

  /// Clear the terminal screen and move cursor to top-left.
  void clearScreen() const;

  /// Move the terminal cursor to the given row and column (1-indexed).
  void moveCursor(int row, int col) const;

  /// Print text in bold.
  void bold(const std::string &text) const;

  /// Print text in a given ANSI colour code (e.g. 32 = green).
  void colour(int code, const std::string &text) const;

  /// Print the Rype ASCII-art logo.
  void printLogo() const;

  /// Print the keybind bar shown at the bottom of the monitoring view.
  void printKeybindBar() const;
};