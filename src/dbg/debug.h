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

#include "exceptions.h"
#include "tools/enumbits.h"

#include <filesystem>
#include <mutex>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <variant>

namespace dbg {

  enum class Severity {
    Info       = 0b00001,
    Warning    = 0b00010,
    Error      = 0b00100,
    Fatal      = 0b01000,
    User       = 0b10000,
    Everything = Info | Warning | Error | Fatal | User
  };
  ENUM_BITFIELD_OPERATORS(Severity);

  /* NOTE: This is a much more elegant way of handling "source",
   * but as of 17.5 it's broken in MSVC.
   * TODO: Once it's fixed and actually works, use this in place of
   * "srcStr/srcLoc".
  using Source = std::variant<std::string_view, std::source_location,
  std::nullopt_t>;
   */

  // These constants are used to prefix messages.

  static constexpr const char* INFOHEAD =
      "\x1b[38;2;0;255;0m\x1b[48;2;10;30;15mINFO";
  static constexpr const char* INFOLIST = "\x1b[38;2;0;255;0m-";
  static constexpr const char* INFOCOLR =
      "\x1b[38;2;0;255;0m\x1b[48;2;10;30;15m";
  static constexpr const char* INFOCOLR_FG = "\x1b[38;2;0;255;0m";
  static constexpr const char* INFOCOLR_BG = "\x1b[48;2;10;30;15m";

  static constexpr const char* WARNHEAD =
      "\x1b[38;2;255;255;0m\x1b[48;2;25;25;5mWARN";
  static constexpr const char* WARNLIST = "\x1b[38;2;255;255;0m~";
  static constexpr const char* WARNCOLR =
      "\x1b[38;2;255;255;0m\x1b[48;2;25;25;5m";
  static constexpr const char* WARNCOLR_FG = "\x1b[38;2;255;255;0m";
  static constexpr const char* WARNCOLR_BG = "\x1b[48;2;25;25;5m";

  static constexpr const char* ERRHEAD =
      "\x1b[38;2;255;0;0m\x1b[48;2;30;15;10mERROR";
  static constexpr const char* ERRLIST = "\x1b[38;2;255;0;0m!";
  static constexpr const char* ERRCOLR =
      "\x1b[38;2;255;0;0m\x1b[48;2;30;15;10m";
  static constexpr const char* ERRCOLR_FG = "\x1b[38;2;255;0;0m";
  static constexpr const char* ERRCOLR_BG = "\x1b[48;2;35;15;10m";

  static constexpr const char* FATALHEAD =
      "\x1b[38;2;0;0;32m\x1b[48;2;255;60;20mFATAL";
  static constexpr const char* FATALLIST = "\x1b[38;2;255;60;20m!!!";
  static constexpr const char* FATALCOLR =
      "\x1b[38;2;0;0;32m\x1b[48;2;255;60;20m";

  static constexpr const char* LUAHEAD =
      "\x1b[38;2;255;0;255m\x1b[48;2;30;10;30mLUA";
  static constexpr const char* LUALIST = "\x1b[38;2;255;0;255m-";
  static constexpr const char* LUACOLR =
      "\x1b[38;2;255;0;255m\x1b[48;2;30;10;30m";
  static constexpr const char* LUACOLR_FG = "\x1b[38;2;255;0;255m";
  static constexpr const char* LUACOLR_BG = "\x1b[48;2;30;10;30m";

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
  void printLine(Severity severity, const std::string_view message);
  /// Prints raw output to the console and log with no newline or formatting.
  void printRaw(Severity severity, const std::string_view message);

  /// Prints a standardized header that goes above other messages in a set.
  void printHeader(dbg::Severity severity, std::string_view headMark,
                   std::optional<std::string_view> srcStr,
                   std::source_location            srcLoc);

  /// Prints a series of lines to the console and log.
  void printLines(Severity severity, const std::string_view listMark,
                  const std::string_view messages);
  void printLines(Severity severity, const std::string_view listMark,
                  const dbg::Exception& exc);

  /// Prints a message to the console and log with formatting.
  template <typename T>
  inline void printMessage(
      Severity severity, const T& messages, const std::string_view listMark,
      std::optional<std::string_view> headMark,
      std::optional<std::string_view> srcStr,
      std::source_location srcLoc = std::source_location::current()) {

    if (headMark)
      printHeader(severity, *headMark, srcStr, srcLoc);

    printLines(severity, listMark, messages);
  }

  /// Prints info message(s) to the console and log.
  template <typename T>
  inline void
      info(const T&                        message,
           std::optional<std::string_view> srcStr = std::nullopt,
           std::source_location srcLoc = std::source_location::current()) {
    printMessage(Severity::Info, message, INFOLIST, INFOHEAD, srcStr, srcLoc);
  }

  /// Prints additional info message(s) to the console and log.
  template <typename T>
  inline void infomore(const T&                        message,
                       std::optional<std::string_view> srcStr = std::nullopt,
                       std::source_location srcLoc = std::source_location()) {
    printMessage(Severity::Info, message, INFOLIST, std::nullopt, srcStr,
                 srcLoc);
  }

  /// Prints warning message(s) to the console and log.
  template <typename T>
  inline void
      warning(const T&                        message,
              std::optional<std::string_view> srcStr = std::nullopt,
              std::source_location srcLoc = std::source_location::current()) {
    printMessage(Severity::Warning, message, WARNLIST, WARNHEAD, srcStr,
                 srcLoc);
  }

  /// Prints additional warning message(s) to the console and log.
  template <typename T>
  inline void warnmore(const T&                        message,
                       std::optional<std::string_view> srcStr = std::nullopt,
                       std::source_location srcLoc = std::source_location()) {
    printMessage(Severity::Warning, message, WARNLIST, std::nullopt, srcStr,
                 srcLoc);
  }

  /// Prints error message(s) to the console and log.
  template <typename T>
  inline void
      error(const T&                        message,
            std::optional<std::string_view> srcStr = std::nullopt,
            std::source_location srcLoc = std::source_location::current()) {
    printMessage(Severity::Error, message, ERRLIST, ERRHEAD, srcStr, srcLoc);
  }

  /// Prints additional error message(s) to the console and log.
  template <typename T>
  inline void errmore(const T&                        message,
                      std::optional<std::string_view> srcStr = std::nullopt,
                      std::source_location srcLoc = std::source_location()) {
    printMessage(Severity::Error, message, ERRLIST, std::nullopt, srcStr,
                 srcLoc);
  }

  /// Prints fatal message(s) to the console and log.
  template <typename T>
  inline void
      fatal(const T&                        message,
            std::optional<std::string_view> srcStr = std::nullopt,
            std::source_location srcLoc = std::source_location::current()) {
    printMessage(Severity::Fatal, message, FATALLIST, FATALHEAD, srcStr,
                 srcLoc);
  }

  /// Prints additional fatal message(s) to the console and log.
  template <typename T>
  inline void fatalmore(const T&                        message,
                        std::optional<std::string_view> srcStr = std::nullopt,
                        std::source_location srcLoc = std::source_location()) {
    printMessage(Severity::Error, message, FATALLIST, std::nullopt, srcStr,
                 srcLoc);
  }

  /// Prints lua message(s) to the console and log.
  template <typename T>
  inline void
      luamsg(const T&                        message,
             std::optional<std::string_view> srcStr = std::nullopt,
             std::source_location srcLoc = std::source_location::current()) {
    printMessage(Severity::User, message, LUALIST, LUAHEAD, srcStr, srcLoc);
  }

  /// Prints lua message(s) to the console and log.
  template <typename T>
  inline void luamore(const T&                        message,
                      std::optional<std::string_view> srcStr = std::nullopt,
                      std::source_location srcLoc = std::source_location()) {
    printMessage(Severity::User, message, LUALIST, std::nullopt, srcStr,
                 srcLoc);
  }
}  // namespace dbg

#endif  // WC_SYS_CLI_H