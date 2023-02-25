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
#include "tools/strtoken.h"

#include <chrono>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <iostream>
#include <regex>

void dbg::printLine(Severity severity, const std::string_view message) {
  cli::print(severity, message, true);
}

void dbg::printRaw(Severity severity, const std::string_view message) {
  cli::print(severity, message, false);
}

void dbg::printHeader(dbg::Severity severity, std::string_view headMark,
                      std::optional<std::string_view> srcStr,
                      std::source_location            srcLoc) {
  // Print the prefix according to the severity.
  dbg::printRaw(severity, headMark);

  // Print the timestamp
  using namespace std::chrono;
  // Print the current date and time according to the system clock.
  auto              now  = system_clock::now();
  auto              nowt = system_clock::to_time_t(now);
  std::stringstream datestr;
  // We want the date and time to be according to the local timezone.
  datestr << std::put_time(std::localtime(&nowt), "%Y-%m-%d %H:%M:");
  // We want to display seconds, but without any decimals.
  dbg::printRaw(severity, fmt::format(" [{}{}]", datestr.str(),
                                      fmt::format("{:%S}", now).substr(0, 2)));

  // If 'srcStr' was provided, print that instead of the formatted source
  // location.
  if (srcStr) {
    dbg::printRaw(severity, fmt::format(" [{}]:{}\n", *srcStr, dbg::CLEAR));
  } else {
    dbg::printRaw(
        severity,
        fmt::format(
            " [{}:{} \"{}\"]:{}\n",
            std::filesystem::path(srcLoc.file_name()).filename().string(),
            srcLoc.line(), srcLoc.function_name(), dbg::CLEAR));
  }
}

/// Implementations for dbg::printLines

void dbg::printLines(Severity severity, const std::string_view listMark,
                     const std::string_view messages) {
  for (auto msg : Tokenizer(messages, "", "\n")) {
    printLine(severity, fmt::format(" {}\x1b[0m {}", listMark, msg));
  }
}
void dbg::printLines(Severity severity, const std::string_view listMark,
                     const dbg::Exception& messages) {
  for (auto& message : messages) {
    for (auto msg : Tokenizer(message, "", "\n")) {
      printLine(severity, fmt::format(" {}\x1b[0m {}", listMark, msg));
    }
  }
}