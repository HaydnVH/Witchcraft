#ifndef WC_SYS_CLI_H
#define WC_SYS_CLI_H

#include "debug.h"
#include <filesystem>
#include <string>
#include <string_view>

namespace cli {

  constexpr const char* LOG_FILENAME = "log.txt";

  //bool initialize();
  void shutdown();

  void print(dbg::MessageSeverity severity, std::string_view message);
  bool popInput(std::string& out);
}

#endif // WC_SYS_CLI_H