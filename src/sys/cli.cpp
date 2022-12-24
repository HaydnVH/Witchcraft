#include "cli.h"

#include <iostream>
#include <regex>

namespace {
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
} // namespace <anon>

namespace cli {

  void printLine(MessageSeverity severity, const std::string_view message) {
    std::cout << message << std::endl;
  }

  void printRaw(MessageSeverity severity, const std::string_view message) {
    std::cout << message;
  }

  void info(const std::optional<const std::string_view>& prefix,
            const std::initializer_list<const std::string_view>& messages) {
    bool first = true;
    for (auto msg : messages) {
      if (first) {
        if (prefix.has_value()) {
          printLine(MessageSeverity::Info,
                    fmt::format("{}{} [{}]: {}{}", INFOCOLR, INFOMARK, prefix.value(), CLEAR, msg));
        } else {
          printLine(MessageSeverity::Info,
                    fmt::format("{}{}: {}{}", INFOCOLR, INFOMARK, CLEAR, msg));
        }
        first = false;
      } else {
        printLine(MessageSeverity::Info,
                  fmt::format("{}{}  {}{}", INFOCOLR, INFOMORE, CLEAR, msg));
      }
    }
  }

  void warning(const std::initializer_list<const std::string_view>& messages) {
    bool first = true;
    for (auto msg : messages) {
      if (first) {
        printLine(MessageSeverity::Warning,
                  fmt::format("{}{}{}{}", WARNCOLR, WARNMARK, CLEAR, msg));
        first = false;
      } else {
        printLine(MessageSeverity::Warning,
                  fmt::format("{}{}{}{}", WARNCOLR, WARNMORE, CLEAR, msg));
      }
    }
  }

  void error(const std::initializer_list<const std::string_view>& messages) {
    bool first = true;
    for (auto msg : messages) {
      if (first) {
        printLine(MessageSeverity::Error,
                  fmt::format("{}{}{}{}", ERRCOLR, ERRMARK, CLEAR, msg));
        first = false;
      } else {
        printLine(MessageSeverity::Error,
                  fmt::format("{}{}{}{}", ERRCOLR, ERRMORE, CLEAR, msg));
      }
    }
  }

  void fatal(const std::initializer_list<const std::string_view>& messages) {
    bool first = true;
    for (auto msg : messages) {
      if (first) {
        printLine(MessageSeverity::Fatal,
                  fmt::format("{}{}{}{}", FATALCOLR, FATALMARK, CLEAR, msg));
        first = false;
      } else {
        printLine(MessageSeverity::Fatal,
                  fmt::format("{}{}{}{}", FATALCOLR, FATALMORE, CLEAR, msg));
      }
    }
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

} // namespace cli