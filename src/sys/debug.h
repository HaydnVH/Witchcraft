/******************************************************************************
 * debug.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified December 2022
 * ---------------------------------------------------------------------------
 * Defines the interface for debugging tools provided by Witchcraft.
 * ALL message outputs should go through here. NEVER use printf of cout!
 *****************************************************************************/
#ifndef WC_SYS_DEBUG_H
#define WC_SYS_DEBUG_H

#include <filesystem>
#include <fmt/core.h>
#include <mutex>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include "tools/enumbits.h"

namespace dbg {

  enum class MessageSeverity {
    Info       = 0b00001,
    Warning    = 0b00010,
    Error      = 0b00100,
    Fatal      = 0b01000,
    User       = 0b10000,
    Everything = Info | Warning | Error | Fatal | User
  };
  ENUM_BITFIELD_OPERATORS(MessageSeverity)

  using Lock = std::unique_lock<std::mutex>;

  // These constants are used to prefix messages.
  static constexpr const char* INFOMARK = "INFO";
  static constexpr const char* INFOMORE = "-";
  static constexpr const char* INFOCOLR =
      "\x1b[38;2;0;255;0m\x1b[48;2;10;30;15m";
  static constexpr const char* INFOCOLR_FG = "\x1b[38;2;0;255;0m";
  static constexpr const char* INFOCOLR_BG = "\x1b[48;2;10;30;15m";

  static constexpr const char* WARNMARK = "WARN";
  static constexpr const char* WARNMORE = "~";
  static constexpr const char* WARNCOLR =
      "\x1b[38;2;255;255;0m\x1b[48;2;25;25;5m";
  static constexpr const char* WARNCOLR_FG = "\x1b[38;2;255;255;0m";
  static constexpr const char* WARNCOLR_BG = "\x1b[48;2;25;25;5m";

  static constexpr const char* ERRMARK = "ERROR";
  static constexpr const char* ERRMORE = "!";
  static constexpr const char* ERRCOLR =
      "\x1b[38;2;255;0;0m\x1b[48;2;30;15;10m";
  static constexpr const char* ERRCOLR_FG = "\x1b[38;2;255;0;0m";
  static constexpr const char* ERRCOLR_BG = "\x1b[48;2;35;15;10m";

  static constexpr const char* FATALMARK = "FATAL";
  static constexpr const char* FATALMORE = "!!!";
  static constexpr const char* FATALCOLR =
      "\x1b[38;2;0;0;32m\x1b[48;2;255;60;20m";

  static constexpr const char* USERMARK = " > ";
  static constexpr const char* USERMORE = " - ";
  static constexpr const char* USERCOLR = "\x1b[38;2;0;255;255m";

  // These constants are used to control color.
  static constexpr const char* CLEAR     = "\x1b[0m";
  static constexpr const char* UL        = "\x1b[4m";
  static constexpr const char* NOUL      = "\x1b[24m";
  static constexpr const char* BLACK     = "\x1b[30m";
  static constexpr const char* DARKRED   = "\x1b[31m";
  static constexpr const char* DARKGREEN = "\x1b[32m";
  static constexpr const char* BROWN     = "\x1b[33m";
  static constexpr const char* DARKBLUE  = "\x1b[34m";
  static constexpr const char* PURPLE    = "\x1b[35m";
  static constexpr const char* DARKCYAN  = "\x1b[36m";
  static constexpr const char* WHITE     = "\x1b[37m";
  static constexpr const char* GREY      = "\x1b[90m";
  static constexpr const char* RED       = "\x1b[91m";
  static constexpr const char* GREEN     = "\x1b[92m";
  static constexpr const char* YELLOW    = "\x1b[93m";
  static constexpr const char* BLUE      = "\x1b[94m";
  static constexpr const char* MAGENTA   = "\x1b[95m";
  static constexpr const char* CYAN      = "\x1b[96m";
  static constexpr const char* BRIGHT    = "\x1b[97m";

  /// Prints a single line to the console and log without additional decoration
  /// or formatting.
  void printLine(MessageSeverity severity, const std::string_view message);
  /// Prints raw output to the console and log with no newline or formatting.
  void printRaw(MessageSeverity severity, const std::string_view message);

  /// Prints a series of 'info' messages to the console and log.
  Lock info(const std::initializer_list<const std::string_view>& messages,
            std::optional<std::string_view> srcOverride = std::nullopt,
            std::source_location src = std::source_location::current());
  /// Prints a single 'info' message to the console and log.
  inline Lock info(const std::string_view          message,
                   std::optional<std::string_view> srcOverride = std::nullopt,
                   std::source_location src = std::source_location::current()) {
    return info({message}, srcOverride, src);
  }
  /// Prints an additional 'info' message to the console and log.
  void infomore(const std::string_view message);

  /// Prints a series of 'warning' messages to the console and log.
  Lock warning(const std::initializer_list<const std::string_view>& messages,
               std::optional<std::string_view> srcOverride = std::nullopt,
               std::source_location src = std::source_location::current());
  /// Prints a single 'warning' message to the console and log.
  inline Lock
      warning(const std::string_view          message,
              std::optional<std::string_view> srcOverride = std::nullopt,
              std::source_location src = std::source_location::current()) {
    return warning({message}, srcOverride, src);
  }
  /// Prints an additional 'warning' message to the console and log.
  void warnmore(const std::string_view message);

  /// Prints a series of 'error' messages to the console and log.
  Lock error(const std::initializer_list<const std::string_view>& messages,
             std::optional<std::string_view> srcOverride = std::nullopt,
             std::source_location src = std::source_location::current());
  /// Prints a single 'error' message to the console and log.
  inline Lock
      error(const std::string_view          message,
            std::optional<std::string_view> srcOverride = std::nullopt,
            std::source_location src = std::source_location::current()) {
    return error({message}, srcOverride, src);
  }
  /// Prints an additional 'error' message to the console and log.
  void errmore(const std::string_view message);

  /// Prints a series of 'fatal' messages to the console and log.
  Lock fatal(const std::initializer_list<const std::string_view>& messages,
             std::optional<std::string_view> srcOverride = std::nullopt,
             std::source_location src = std::source_location::current());
  /// Prints a single 'fatal' message to the console and log.
  inline Lock fatal(const std::string_view          message,
            std::optional<std::string_view> srcOverride = std::nullopt,
            std::source_location src = std::source_location::current()) {
    return fatal({message}, srcOverride, src);
  }
  /// Prints an additional 'fatal' message to the console and log.
  void fatalmore(const std::string_view message);

  /// Prints a series of 'user' messages to the console and log.
  void user(const std::initializer_list<const std::string_view>& messages);
  /// Prints a single 'user' message to the console and log.
  inline void user(const std::string_view message) { user({message}); }

}  // namespace cli

#endif  // WC_SYS_CLI_H