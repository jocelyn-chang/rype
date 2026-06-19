/**
 * @file terminal_ui.cpp
 * @brief Implementation of the TerminalUI class.
 *
 * @details
 * Uses ANSI escape codes for colours, cursor movement, and screen clearing.
 * Raw terminal mode (termios) is used for single-keypress input so the user
 * does not need to press Enter after each keybind.
 *
 * Supported terminals: macOS Terminal, iTerm2, Linux console, Raspberry Pi
 * terminal. Windows CMD / PowerShell are not supported.
 *
 * @author Rype Team 1
 */

#include "terminal_ui.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// ANSI escape code constants
// ---------------------------------------------------------------------------

/// Reset all formatting attributes.
static const std::string RESET  = "\033[0m";
/// Bold text.
static const std::string BOLD   = "\033[1m";
/// Dim text.
static const std::string DIM    = "\033[2m";
/// Green foreground.
static const std::string GREEN  = "\033[32m";
/// Yellow foreground.
static const std::string YELLOW = "\033[33m";
/// Red foreground.
static const std::string RED    = "\033[31m";
/// Cyan foreground.
static const std::string CYAN   = "\033[36m";
/// White foreground.
static const std::string WHITE  = "\033[37m";
/// Blue foreground.
static const std::string BLUE   = "\033[34m";
/// Clear screen and move to home position.
static const std::string CLEAR  = "\033[2J\033[H";

// ---------------------------------------------------------------------------
// Constructor — populate the plant catalogue
// ---------------------------------------------------------------------------

/**
 * @brief Initialise the TerminalUI and build the built-in plant catalogue.
 *
 * @details
 * Each PlantProfile entry defines the plant's name, emoji icon, ideal
 * moisture range, a short watering tip, and a fun fact for the info panel.
 * Add more entries here to expand the plant selection menu.
 */
TerminalUI::TerminalUI() {
  plants_ = {
    {
      "Aloe Vera",
      "🌵",
      66.2, 91.5,
      "Keep moisture in the calibrated Aloe range (RAW 300-380).",
      "Aloe stores water in its leaves, so steady moderate moisture works best."
    },
    {
      "Other Plant",
      "🌿",
      66.2, 98.7,
      "Keep moisture in the calibrated Other range (RAW 277-380).",
      "Most non-succulent houseplants prefer moisture near the wet side of this range."
    }
  };
}

// ---------------------------------------------------------------------------
// ANSI helpers
// ---------------------------------------------------------------------------

/**
 * @brief Clear the terminal screen and move the cursor to the top-left.
 */
void TerminalUI::clearScreen() const {
  std::cout << CLEAR;
  std::cout.flush();
}

/**
 * @brief Move the terminal cursor to the specified row and column.
 * @param row 1-indexed row number.
 * @param col 1-indexed column number.
 */
void TerminalUI::moveCursor(int row, int col) const {
  std::cout << "\033[" << row << ";" << col << "H";
}

/**
 * @brief Print text in bold formatting.
 * @param text The string to display in bold.
 */
void TerminalUI::bold(const std::string &text) const {
  std::cout << BOLD << text << RESET;
}

/**
 * @brief Print text in the specified ANSI foreground colour.
 * @param code ANSI colour code (e.g. 32 = green, 31 = red, 33 = yellow).
 * @param text The string to display in that colour.
 */
void TerminalUI::colour(int code, const std::string &text) const {
  std::cout << "\033[" << code << "m" << text << RESET;
}

// ---------------------------------------------------------------------------
// Logo
// ---------------------------------------------------------------------------

/**
 * @brief Print the Rype ASCII-art logo to stdout.
 *
 * @details
 * Rendered in green with a cyan subtitle. Called from showSplash() and
 * at the top of the plant-selection menu.
 */
void TerminalUI::printLogo() const {
  std::cout << GREEN << BOLD;
  std::cout << R"(
  ██████╗ ██╗   ██╗██████╗ ███████╗
  ██╔══██╗╚██╗ ██╔╝██╔══██╗██╔════╝
  ██████╔╝ ╚████╔╝ ██████╔╝█████╗
  ██╔══██╗  ╚██╔╝  ██╔═══╝ ██╔══╝
  ██║  ██║   ██║   ██║     ███████╗
  ╚═╝  ╚═╝   ╚═╝   ╚═╝     ╚══════╝
)" << RESET;
  std::cout << CYAN << "        Plant Monitoring System\n" << RESET;
  std::cout << DIM  << "           CS 3307B — Team 1\n\n" << RESET;
}

// ---------------------------------------------------------------------------
// Keybind bar
// ---------------------------------------------------------------------------

/**
 * @brief Print the keybind hint bar shown during live monitoring.
 *
 * @details
 * Displays a single line of keyboard shortcuts at the bottom of the
 * monitoring view so the user always knows what keys are available.
 */
void TerminalUI::printKeybindBar() const {
  std::cout << "\n";
  std::cout << DIM << std::string(70, '-') << RESET << "\n";
  std::cout << BOLD << "  Keybinds: " << RESET
            << CYAN   << "[I]" << RESET << " Plant Info   "
            << GREEN  << "[V]" << RESET << " Vision Check   "
            << YELLOW << "[H]" << RESET << " Help   "
            << RED    << "[Q]" << RESET << " Quit\n";
}

// ---------------------------------------------------------------------------
// Splash screen
// ---------------------------------------------------------------------------

/**
 * @brief Show an animated splash screen and block for ~1.5 seconds.
 *
 * @details
 * Clears the screen, prints the logo, and then animates a short loading
 * message before returning control to the caller.
 */
void TerminalUI::showSplash() const {
  clearScreen();
  std::cout << "\n\n\n";
  printLogo();

  // Animate a simple loading bar
  std::cout << "  Initialising";
  for (int i = 0; i < 6; ++i) {
    std::cout << "." << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  std::cout << " " << GREEN << "ready!" << RESET << "\n\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
}

// ---------------------------------------------------------------------------
// Plant selection menu
// ---------------------------------------------------------------------------

/**
 * @brief Render the plant-selection menu and return the user's choice.
 *
 * @details
 * Prints each plant in the catalogue with a number key, emoji, name, and
 * ideal moisture range. The user presses a digit key to confirm. Invalid
 * input is ignored and the prompt is re-shown.
 *
 * @return The PlantProfile the user selected.
 */
PlantProfile TerminalUI::selectPlant() const {
  while (true) {
    clearScreen();
    std::cout << "\n";
    printLogo();

    std::cout << BOLD << "  Select your plant:\n\n" << RESET;

    for (int i = 0; i < static_cast<int>(plants_.size()); ++i) {
      const auto &p = plants_[i];
      std::cout << "  " << CYAN << BOLD << "[ " << (i + 1) << " ]" << RESET
                << "  " << p.emoji << "  " << BOLD << p.name << RESET
                << DIM
                << "  (ideal moisture: "
                << static_cast<int>(p.minMoisture) << "% – "
                << static_cast<int>(p.maxMoisture) << "%)"
                << RESET << "\n\n";
    }

    std::cout << DIM << std::string(55, '-') << RESET << "\n";
    std::cout << "  Press a number key to select: ";
    std::cout.flush();

    char key = getKey();
    int choice = key - '0';

    if (choice >= 1 && choice <= static_cast<int>(plants_.size())) {
      const PlantProfile &selected = plants_[choice - 1];

      // Confirmation screen
      clearScreen();
      std::cout << "\n\n";
      printLogo();
      std::cout << "  " << GREEN << BOLD << "Selected: " << RESET
                << selected.emoji << "  " << BOLD << selected.name << RESET
                << "\n\n";
      std::cout << DIM << "  Ideal moisture range: "
                << static_cast<int>(selected.minMoisture) << "% – "
                << static_cast<int>(selected.maxMoisture) << "%\n" << RESET;
      std::cout << "\n  " << DIM << "Starting monitor in 2 seconds..." << RESET << "\n";
      std::this_thread::sleep_for(std::chrono::seconds(2));

      return selected;
    }
    // Invalid key — loop and redraw
  }
}

// ---------------------------------------------------------------------------
// Monitor header
// ---------------------------------------------------------------------------

/**
 * @brief Print the static header for the live monitoring table.
 *
 * @param plant The selected plant, shown in the header title.
 */
void TerminalUI::printMonitorHeader(const PlantProfile &plant) const {
  clearScreen();
  std::cout << "\n";

  // Title bar
  std::cout << GREEN << BOLD
            << "  Rype Monitor" << RESET
            << "  —  " << plant.emoji << "  " << BOLD << plant.name << RESET
            << DIM
            << "  (ideal: "
            << static_cast<int>(plant.minMoisture) << "% – "
            << static_cast<int>(plant.maxMoisture) << "%)"
            << RESET << "\n\n";

  // Table header
  std::cout << BOLD
            << std::left
            << "  " << std::setw(10) << "Time"
            << "| " << std::setw(10) << "Raw ADC"
            << "| " << std::setw(14) << "Moisture %"
            << "| " << std::setw(12) << "Status"
            << "| " << "CV Health"
            << RESET << "\n";
  std::cout << DIM << "  " << std::string(68, '-') << RESET << "\n";

  printKeybindBar();
  std::cout << "\n";
}

// ---------------------------------------------------------------------------
// Live reading row
// ---------------------------------------------------------------------------

/**
 * @brief Print one sensor reading as a table row with colour-coded status.
 *
 * @details
 * Status is shown as one of: "TOO WET", "NEEDS WATER", or "IDEAL RANGE".
 * The moisture percentage is colour-coded: red below 20%, yellow 20–40%,
 * green above 40%.
 *
 * @param ts          Unix timestamp.
 * @param rawAdc      Raw ADC count.
 * @param moisturePct Filtered moisture percentage.
 * @param needsWater  Whether moisture is below the plant's ideal range.
 * @param tooWet      Whether moisture is above the plant's ideal range.
 */
void TerminalUI::printReading(std::int64_t ts, int rawAdc, double moisturePct,
                               bool needsWater, bool tooWet,
                               const std::string &cvHealth) const {
  std::time_t  t  = static_cast<std::time_t>(ts);
  std::tm     *lt = std::localtime(&t);

  char timeBuf[16];
  std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", lt);

  // Choose moisture colour
  std::string mColour = GREEN;
  if (moisturePct < 20.0)      mColour = RED;
  else if (moisturePct < 40.0) mColour = YELLOW;

  // Choose status colour and label
  std::string statusColour = GREEN;
  std::string statusLabel  = "IDEAL RANGE";

  if (tooWet) {
    statusColour = YELLOW;
    statusLabel = "TOO WET";
  } else if (needsWater) {
    statusColour = RED;
    statusLabel = "NEEDS WATER";
  }

  std::string cvColour = WHITE;
  if (cvHealth == "HEALTHY") {
    cvColour = GREEN;
  } else if (cvHealth == "UNHEALTHY") {
    cvColour = RED;
  } else if (cvHealth == "RUNNING") {
    cvColour = YELLOW;
  } else if (cvHealth == "ERROR") {
    cvColour = RED;
  }

  std::cout << "  " << std::left
            << std::setw(10) << timeBuf
            << "| RAW " << std::setw(6) << rawAdc
            << "| " << mColour << BOLD
            << std::setw(4) << static_cast<int>(moisturePct)
            << "%" << RESET
            << std::setw(9) << ""
            << "| " << statusColour << BOLD << std::setw(12) << statusLabel << RESET
            << "| " << cvColour << BOLD << cvHealth << RESET
            << "\n";

  std::cout.flush();
}

// ---------------------------------------------------------------------------
// Info panel
// ---------------------------------------------------------------------------

/**
 * @brief Show the plant info overlay panel.
 *
 * @details
 * Clears the screen, renders a bordered info card with the plant's details,
 * then waits for any key before returning and redrawing the monitor view.
 *
 * @param plant       The currently monitored plant profile.
 * @param moisturePct Current sensor reading to display.
 */
void TerminalUI::showInfoPanel(const PlantProfile &plant,
                                double moisturePct,
                                const std::string &cvHealth) const {
  clearScreen();
  std::cout << "\n\n";
  printLogo();

  std::cout << CYAN << BOLD
            << "  Plant Info — " << plant.emoji << " " << plant.name
            << "\n" << RESET;
  std::cout << DIM << "  " << std::string(52, '=') << "\n\n" << RESET;

  std::cout << BOLD << "  Current moisture:  " << RESET;

  std::string mColour = GREEN;
  if (moisturePct < 20.0)      mColour = RED;
  else if (moisturePct < 40.0) mColour = YELLOW;

  std::cout << mColour << BOLD << static_cast<int>(moisturePct)
            << "%" << RESET << "\n\n";

  std::cout << BOLD << "  Ideal range:       " << RESET
            << static_cast<int>(plant.minMoisture) << "% – "
            << static_cast<int>(plant.maxMoisture) << "%\n\n";

  std::string cvColour = WHITE;
  if (cvHealth == "HEALTHY") {
    cvColour = GREEN;
  } else if (cvHealth == "UNHEALTHY") {
    cvColour = RED;
  } else if (cvHealth == "RUNNING") {
    cvColour = YELLOW;
  } else if (cvHealth == "ERROR") {
    cvColour = RED;
  }

  std::cout << BOLD << "  Vision health:     " << RESET
            << cvColour << BOLD << cvHealth << RESET << "\n\n";

  std::cout << BOLD << "  Watering tip:\n" << RESET
            << "  " << plant.wateringTip << "\n\n";

  std::cout << BOLD << "  Fun fact:\n" << RESET
            << "  " << DIM << plant.funFact << RESET << "\n\n";

  std::cout << DIM << "  " << std::string(52, '=') << "\n\n" << RESET;
  std::cout << "  Press any key to return to monitoring...";
  std::cout.flush();

  getKey();
}

// ---------------------------------------------------------------------------
// Help screen
// ---------------------------------------------------------------------------

/**
 * @brief Show the keybind help screen overlay.
 *
 * @details
 * Displays all available keyboard shortcuts in a formatted panel.
 * Press any key to dismiss and return to the monitoring view.
 */
void TerminalUI::showHelpScreen() const {
  clearScreen();
  std::cout << "\n\n";
  printLogo();

  std::cout << YELLOW << BOLD << "  Keyboard Shortcuts\n" << RESET;
  std::cout << DIM << "  " << std::string(52, '=') << "\n\n" << RESET;

  auto row = [](const std::string &key, const std::string &desc,
                const std::string &keyColour) {
    std::cout << "  " << keyColour << BOLD << std::setw(6) << key << RESET
              << "  " << desc << "\n\n";
  };

  row("[I]", "Show plant info — current moisture, ideal range,\n"
             "        watering tip, and a fun fact.", CYAN);
  row("[V]", "Run computer-vision health check (camera snapshot).", GREEN);
  row("[H]", "Show this help screen.", YELLOW);
  row("[Q]", "Quit the plant monitor gracefully.", RED);

  std::cout << DIM << "  " << std::string(52, '=') << "\n\n" << RESET;
  std::cout << "  Press any key to return to monitoring...";
  std::cout.flush();

  getKey();
}

// ---------------------------------------------------------------------------
// Raw keypress input
// ---------------------------------------------------------------------------

/**
 * @brief Read a single keypress without requiring the user to press Enter.
 *
 * @details
 * Saves the current terminal attributes, switches to raw (non-canonical)
 * mode with no echo, reads one byte, then restores the original settings.
 * The returned character is converted to lowercase for consistent matching.
 *
 * @return The lowercase character of the key pressed.
 */
char TerminalUI::getKey() const {
  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;

  // Disable canonical mode and echo
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  char c = 0;
  read(STDIN_FILENO, &c, 1);

  // Restore original terminal settings
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}