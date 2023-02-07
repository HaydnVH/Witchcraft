/******************************************************************************
 * debug.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified December 2022
 * ---------------------------------------------------------------------------
 * Implements Witchcraft's debugging tools.
 *****************************************************************************/
#include "debug.h"
#include "cli.h"

#include <chrono>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <iostream>
#include <regex>

/// Grab the mutex from "cli.cpp" so we can synchronize CLI behaviour.
extern std::mutex cliMutex_g;

namespace {

  bool shouldPrintTime_s = true;

  void printStandardPrefix(dbg::MessageSeverity            severity,
                           std::optional<std::string_view> srcOverride,
                           std::source_location            src) {
    // Print the prefix according to the severity.
    switch (severity) {
    case dbg::MessageSeverity::Info:
      dbg::printRaw(severity,
                    fmt::format("{}{} ", dbg::INFOCOLR, dbg::INFOMARK));
      break;
    case dbg::MessageSeverity::Warning:
      dbg::printRaw(severity,
                    fmt::format("{}{} ", dbg::WARNCOLR, dbg::WARNMARK));
      break;
    case dbg::MessageSeverity::Error:
      dbg::printRaw(severity, fmt::format("{}{} ", dbg::ERRCOLR, dbg::ERRMARK));
      break;
    case dbg::MessageSeverity::Fatal:
      dbg::printRaw(severity,
                    fmt::format("{}{} ", dbg::FATALCOLR, dbg::FATALMARK));
      break;
    default: break;
    }

    if (shouldPrintTime_s) {
      // Print the current date and time according to the system clock.
      auto now = std::chrono::system_clock::now();
      dbg::printRaw(severity, fmt::format("[{:%Y-%m-%d %H:%M:%S}] ", now));
    }

    // If 'srcOverride' was provided, print that instead of the formatted source
    // location.
    if (srcOverride) {
      dbg::printRaw(severity,
                    fmt::format("[{}]:{}\n", srcOverride.value(), dbg::CLEAR));
    } else {
      dbg::printRaw(
          severity,
          fmt::format(
              "[{}:{} \"{}\"]:{}\n",
              std::filesystem::path(src.file_name()).filename().string(),
              src.line(), src.function_name(), dbg::CLEAR));
    }
  }
}  // namespace

namespace dbg {

  void printLine(MessageSeverity severity, const std::string_view message) {
    cli::print(severity, message, true);
  }
  
  void printRaw(MessageSeverity severity, const std::string_view message) {
    cli::print(severity, message, false);
  }

  Lock info(const std::initializer_list<const std::string_view>& messages,
            std::optional<std::string_view>                      srcOverride,
            std::source_location                                 src) {
    Lock lock(cliMutex_g);
    printStandardPrefix(MessageSeverity::Info, srcOverride, src);
    for (auto msg : messages) { infomore(msg); }
    return std::move(lock);
  }
  void infomore(const std::string_view message) {
    printLine(MessageSeverity::Info,
              fmt::format(" {}{}{} {}", INFOCOLR_FG, INFOMORE, CLEAR, message));
  }

  Lock warning(const std::initializer_list<const std::string_view>& messages,
               std::optional<std::string_view>                      srcOverride,
               std::source_location                                 src) {
    Lock lock(cliMutex_g);
    printStandardPrefix(MessageSeverity::Warning, srcOverride, src);
    for (auto msg : messages) { warnmore(msg); }
    return std::move(lock);
  }
  void warnmore(const std::string_view message) {
    printLine(MessageSeverity::Warning,
              fmt::format(" {}{}{} {}", WARNCOLR_FG, WARNMORE, CLEAR, message));
  }

  Lock error(const std::initializer_list<const std::string_view>& messages,
             std::optional<std::string_view>                      srcOverride,
             std::source_location                                 src) {
    Lock lock(cliMutex_g);
    printStandardPrefix(MessageSeverity::Error, srcOverride, src);
    for (auto msg : messages) { errmore(msg); }
    return std::move(lock);
  }
  void errmore(const std::string_view message) {
    printLine(MessageSeverity::Error,
              fmt::format(" {}{}{} {}", ERRCOLR_FG, ERRMORE, CLEAR, message));
  }

  Lock fatal(const std::initializer_list<const std::string_view>& messages,
             std::optional<std::string_view>                      srcOverride,
             std::source_location                                 src) {
    Lock lock(cliMutex_g);
    printStandardPrefix(MessageSeverity::Fatal, srcOverride, src);
    for (auto msg : messages) { fatalmore(msg); }
    return std::move(lock);
  }
  void fatalmore(const std::string_view message) {
    printLine(MessageSeverity::Fatal,
              fmt::format(" {}{}{} {}", ERRCOLR_FG, FATALMORE, CLEAR, message));
  }

  void user(const std::initializer_list<const std::string_view>& messages) {
    bool first = true;
    for (auto msg : messages) {
      if (first) {
        printLine(MessageSeverity::User,
                  fmt::format("{}{}{}{}", USERCOLR, USERMARK, CLEAR, msg));
        first = false;
      } else {
        printLine(MessageSeverity::User,
                  fmt::format("{}{}{}{}", USERCOLR, USERMORE, CLEAR, msg));
      }
    }
  }

}  // namespace cli