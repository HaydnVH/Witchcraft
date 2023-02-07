/******************************************************************************
 * cli.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified December 2022
 * ---------------------------------------------------------------------------
 * Defines the interface for the command-line-interface system.
 *****************************************************************************/
#ifndef WC_SYS_CLI_H
#define WC_SYS_CLI_H

#include "debug.h"
#include <filesystem>
#include <string>
#include <string_view>

namespace cli {

  /// The name of the file where the log should be saved (in the user directory).
  constexpr const char* LOG_FILENAME = "log.txt";

  /// Returns the terminal back to its default state and closes the log file.
  /// Unlike initialization, this must be done manually before the application closes.
  void shutdown();

  /// Prints a single message.
  /// @param severity: The severity of the message so it can be filtered by settings.
  /// @param message: The message to print.
  /// @param endl: Whether to insert an "end of line" after the message.
  void print(dbg::MessageSeverity severity, std::string_view message, bool endl);

  /// Gets a single line of input that the user has entered into the terminal.
  /// @param out: The string where the line of input should be stored.
  /// @returns true if a message was popped from the queue, false if the queue was empty.
  bool popInput(std::string& out);
}

#endif // WC_SYS_CLI_H