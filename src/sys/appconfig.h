/******************************************************************************
 * appconfig.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified December 2022
 ******************************************************************************
 * Defines a handful of application-wide constants such as name and version.
 *****************************************************************************/
#ifndef WC_SYS_APPCONFIG_H
#define WC_SYS_APPCONFIG_H

namespace wc {

  /* The following fields are intended to be modified on a per-project basis. */

  /// The name of the application.
  constexpr const char* APP_NAME = "Witchcraft Project";
  /// The name of the company responsible for the application.
  constexpr const char* COMPANY_NAME = "hvh";
  /// The major version of the application,
  /// incremented during compatability-breaking major releases.
  constexpr const int APP_MAJORVER = 0;
  /// The minor version of the application,
  /// incremented for notable feature releases.
  constexpr const int APP_MINORVER = 0;
  /// The patch version of the application,
  /// incremented for minor fixes and patches.
  constexpr const int APP_PATCHVER = 0;
  /// The version of the application, in string form.
  constexpr const char* APP_VERSION = "0.0.0";

  /* The following fields should not be modified unless updating the engine. */

  /// The name of the engine used to create the application.
  constexpr const char* ENGINE_NAME = "Witchcraft";
  /// The major version of the engine,
  /// incremented during compatability-breaking major releases.
  constexpr const int ENGINE_MAJORVER = 0;
  /// The minor version of the engine,
  /// incremented for notable feature releases.
  constexpr const int ENGINE_MINORVER = 13;
  /// The patch version of the engine,
  /// incremented for minor fixes and patches.
  constexpr const int ENGINE_PATCHVER = 0;
  /// The version of the application, in string form.
  constexpr const char* ENGINE_VERSION = "0.13.0";

}  // namespace wc

#endif  // WC_SYS_APPCONFIG_H