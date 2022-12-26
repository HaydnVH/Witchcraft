#include "cli.h"

#include <chrono>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <iostream>
#include <mutex>
#include <regex>
#include <thread>

namespace {

  bool shouldPrintTime_s = true;

  std::mutex cliMutex_s;

  // This regex ought to find all possible ANSI escape sequences.
  static constexpr const char*
      ANSI_ESC_REGEX_STR("\033\\[((?:\\d|;)*)([a-zA-Z])");

  /// Removes ANSI escape sequences from a string.
  void stripAnsi(std::string& str) {
    std::regex r(ANSI_ESC_REGEX_STR);
    str = std::regex_replace(str, r, "");
  }

  /// Creates a copy of a string with ANSI escape sequences removed.
  std::string strippedAnsi(const std::string& str) {
    std::regex r(ANSI_ESC_REGEX_STR);
    return std::regex_replace(str, r, "");
  }

  void printStandardPrefix(cli::MessageSeverity            severity,
                           std::optional<std::string_view> srcOverride,
                           std::source_location            src) {
    // Print the prefix according to the severity.
    switch (severity) {
    case cli::MessageSeverity::Info:
      cli::printRaw(severity,
                    fmt::format("{}{} ", cli::INFOCOLR, cli::INFOMARK));
      break;
    case cli::MessageSeverity::Warning:
      cli::printRaw(severity,
                    fmt::format("{}{} ", cli::WARNCOLR, cli::WARNMARK));
      break;
    case cli::MessageSeverity::Error:
      cli::printRaw(severity, fmt::format("{}{} ", cli::ERRCOLR, cli::ERRMARK));
      break;
    case cli::MessageSeverity::Fatal:
      cli::printRaw(severity,
                    fmt::format("{}{} ", cli::FATALCOLR, cli::FATALMARK));
      break;
    default: break;
    }

    if (shouldPrintTime_s) {
      // Print the current date and time according to the system clock.
      auto now = std::chrono::system_clock::now();
      cli::printRaw(severity, fmt::format("[{:%Y-%m-%d %H:%M:%S}] ", now));
    }

    // If 'srcOverride' was provided, print that instead of the formatted source
    // location.
    if (srcOverride) {
      cli::printRaw(severity,
                    fmt::format("[{}]:{}\n", srcOverride.value(), cli::CLEAR));
    } else {
      cli::printRaw(
          severity,
          fmt::format(
              "[{}:{} \"{}\"]:{}\n",
              std::filesystem::path(src.file_name()).filename().string(),
              src.line(), src.function_name(), cli::CLEAR));
    }
  }
}  // namespace

namespace cli {

  void printLine(MessageSeverity severity, const std::string_view message) {
    printRaw(severity, fmt::format("{}\n", message));
  }

  void printRaw(MessageSeverity severity, const std::string_view message) {
    std::cout << message;
  }

  Lock info(const std::initializer_list<const std::string_view>& messages,
            std::optional<std::string_view>                      srcOverride,
            std::source_location                                 src) {
    Lock lock(cliMutex_s);
    printStandardPrefix(MessageSeverity::Info, srcOverride, src);
    for (auto msg : messages) { infomore(lock, msg); }
    return lock;
  }
  void infomore(Lock&, const std::string_view message) {
    printLine(MessageSeverity::Info, fmt::format(
              " {}{}{} {}", INFOCOLR_FG, INFOMORE, CLEAR, message));
  }

  Lock warning(const std::initializer_list<const std::string_view>& messages,
               std::optional<std::string_view>                      srcOverride,
               std::source_location                                 src) {
    Lock lock(cliMutex_s);
    printStandardPrefix(MessageSeverity::Warning, srcOverride, src);
    for (auto msg : messages) { warnmore(lock, msg); }
    return lock;
  }
  void warnmore(Lock&, const std::string_view message) {
    printLine(MessageSeverity::Warning, fmt::format(
              " {}{}{} {}", WARNCOLR_FG, WARNMORE, CLEAR, message));
  }

  Lock error(const std::initializer_list<const std::string_view>& messages,
             std::optional<std::string_view>                      srcOverride,
             std::source_location                                 src) {
    Lock lock(cliMutex_s);
    printStandardPrefix(MessageSeverity::Error, srcOverride, src);
    for (auto msg : messages) { errmore(lock, msg); }
    return lock;
  }
  void errmore(Lock&, const std::string_view message) {
    printLine(MessageSeverity::Error, fmt::format(
              " {}{}{} {}", ERRCOLR_FG, ERRMORE, CLEAR, message));
  }

  Lock fatal(const std::initializer_list<const std::string_view>& messages,
             std::optional<std::string_view>                      srcOverride,
             std::source_location                                 src) {
    Lock lock(cliMutex_s);
    printStandardPrefix(MessageSeverity::Fatal, srcOverride, src);
    for (auto msg : messages) { fatalmore(lock, msg); }
    return lock;
  }
  void fatalmore(Lock&, const std::string_view message) {
    printLine(MessageSeverity::Fatal, fmt::format(
              " {}{}{} {}", ERRCOLR_FG, FATALMORE, CLEAR, message));
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