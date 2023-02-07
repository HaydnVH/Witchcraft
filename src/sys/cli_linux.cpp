/******************************************************************************
 * cli_linux.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified December 2022
 * ---------------------------------------------------------------------------
 * Implements the command-line-interface system on Linux.
 *****************************************************************************/
#ifdef PLATFORM_LINUX
#include "cli.h"
#include "debug.h"

#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <sys/paths.h>

/// Although this is technically a "global", it should only ever be 'extern'd
/// from "debug.cpp". This mutex is used to synchronize CLI behaviour.
std::mutex cliMutex_g;

namespace {

  bool                     initialized_s = false;
  std::ofstream            logFile_s;
  std::vector<std::string> crashReports_s;
  std::queue<std::string>  consoleQueue_s;
  std::queue<std::tuple<dbg::MessageSeverity, std::string>> preinitQueue_s;
  std::string                                               lastMessage_s = "";

  struct {
    dbg::MessageSeverity stdoutFilter  = dbg::MessageSeverity::Everything;
    dbg::MessageSeverity logfileFilter = dbg::MessageSeverity::Everything;
    dbg::MessageSeverity consoleFilter = dbg::MessageSeverity::Everything;
    bool                 makeConsole   = true;
    bool                 allowCheats   = true;
    bool                 modified      = false;
  } myConfig;

  std::thread       cinThread_s;
  bool              inThreadRunning_s = true;
  std::atomic<bool> updateInLine_s    = true;

  void terminalInputThread() {
    while (inThreadRunning_s) {

    }
  }

  bool initialize() {
    // Open the log file.
    std::filesystem::path logpath = wc::getUserPath() / LOG_FILENAME;
    logFile_s.open(logpath, std::ios::out);
  }

} // namespace anon

void cli::shutdown() {}

void cli::print(dbg::MessageSeverity severity, std::string_view message,
  bool endl) {

}

bool popInput(std::string& out) {}

#endif // PLATFORM_LINUX