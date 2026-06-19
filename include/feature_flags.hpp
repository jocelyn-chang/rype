/**
 * @file feature_flags.hpp
 * @brief Central feature-flag definitions for the plant monitoring system.
 *
 * Each flag has TWO forms:
 *   1. A preprocessor macro  (FEATURE_DISCORD_NOTIFICATIONS, etc.)
 *      — used to guard #include directives and heavy library headers.
 *   2. A constexpr bool      (feature_flags::ENABLE_DISCORD_NOTIFICATIONS,
 * etc.) — used inside function bodies with  if constexpr(...)  for zero-cost
 * guards.
 *
 * Both forms must agree.  Flip the 0/1 macro *and* the true/false constexpr
 * together when enabling or disabling a feature.
 *
 * HOW TO ADD A NEW FLAG:
 *   1. Add a  #define FEATURE_YOUR_FLAG 0  macro below.
 *   2. Add a  inline constexpr bool ENABLE_YOUR_FLAG = false;  in the
 * namespace.
 *   3. Guard the feature's includes with  #if FEATURE_YOUR_FLAG  in the
 *      relevant .cpp files.
 *   4. Guard initialisation and runtime calls with
 *         if constexpr (feature_flags::ENABLE_YOUR_FLAG) { ... }
 *      in main.cpp.
 *   5. If the feature needs an external library, add a conditional
 *      find_package / target_link_libraries block in CMakeLists.txt.
 */

#ifndef FEATURE_FLAGS_HPP
#define FEATURE_FLAGS_HPP

// ---- Preprocessor flags (for guarding #include directives) ----------------
// Set to 1 to enable the feature, 0 to disable.

#define FEATURE_DISCORD_NOTIFICATIONS 1
#define FEATURE_LED_ALERTS 1
#define FEATURE_STATUS_WEB 0
#define FEATURE_LED_SOLID_ON_TEST 0

// ---- Constexpr flags (for if-constexpr guards in function bodies) ---------

namespace feature_flags {

/// Enable Discord webhook notifications (requires libcurl at link time).
inline constexpr bool ENABLE_DISCORD_NOTIFICATIONS =
    FEATURE_DISCORD_NOTIFICATIONS;

/// Enable GPIO LED alerts via sysfs (requires Raspberry Pi GPIO access).
inline constexpr bool ENABLE_LED_ALERTS = FEATURE_LED_ALERTS;

/// Force LED to remain solid ON while the app is running (hardware test mode).
inline constexpr bool ENABLE_LED_SOLID_ON_TEST = FEATURE_LED_SOLID_ON_TEST;

/// Enable the local web status dashboard (uses bundled httplib).
inline constexpr bool ENABLE_STATUS_WEB = FEATURE_STATUS_WEB;

} // namespace feature_flags

#endif // FEATURE_FLAGS_HPP
