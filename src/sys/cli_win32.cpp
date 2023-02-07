/******************************************************************************
 * cli_win32.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2022 - present
 * Last modified December 2022
 * ---------------------------------------------------------------------------
 * Implements the command-line-interface system on Windows.
 *****************************************************************************/
#ifdef PLATFORM_WIN32
#include "cli.h"
#include "debug.h"

#include <atomic>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <regex>
#include <string>
#include <sys/paths.h>
#include <thread>
#include "tools/utf.h"
#include <tuple>
#include <vector>

#include <Windows.h>

/// Although this is technically a "global", it should only ever be 'extern'd from "debug.cpp".
/// This mutex is used to synchronize CLI behaviour.
std::mutex cliMutex_g;

namespace {

  bool                                      initialized_s = false;
  std::ofstream                             logFile_s;
  std::vector<std::string>                  crashReports_s;
  std::queue<std::string>                   consoleQueue_s;
  std::queue<std::tuple<dbg::MessageSeverity, std::string>>  preinitQueue_s;
  std::string                               lastMessage_s = "";

  struct {
    dbg::MessageSeverity stdoutFilter  = dbg::MessageSeverity::Everything;
    dbg::MessageSeverity logfileFilter = dbg::MessageSeverity::Everything;
    dbg::MessageSeverity consoleFilter = dbg::MessageSeverity::Everything;
    bool makeConsole   = true;
    bool allowCheats   = true;
    bool modified      = false;
  } myConfig;

  std::thread       cinThread_s;
  bool              inThreadRunning_s = true;
  std::atomic<bool> updateInLine_s = true;

  HANDLE hcout, hcin;
  DWORD  original_hcout_mode, original_hcin_mode;

  // Saves the cursor position in memory.
  constexpr const char DECSC[] = {'\x1b', '7', '\0'};

  // Restores the cursor position from memory.
  constexpr const char DECSR[] = {'\x1b', '8', '\0'};

  // Moves the cursor to the beginning of the next line.
  constexpr const char* IL = "\x1b[1L";

  // Erases characters after the cursor.
  constexpr const char* ED = "\x1b[0J";

  // Wakes up the input thread if it's blocked on stdin.
  constexpr const char* WAKEUP = "\x1b[6n";

  // The main function for the console input thread.
  void terminalInputThread() {

    // 'memory_buffer' remember all of the lines that we've previously input.
    // It includes the line we're currently on, so it starts with 1 entry.
    std::vector<std::u32string> memory_buffer(1);
    // The index of the current memory buffer starts with 0.
    int current_memory_buffer = 0;
    // 'input_buffer' points to which memory buffer we're currently on.
    std::u32string* input_buffer = &memory_buffer.back();
    std::u32string  temp;
    std::string     qstr         = "";
    std::string     echostr      = "";
    std::string     inputstr     = "";
    int             input_cursor = 0;

    while (inThreadRunning_s) {

      // This chunk of code reads all of the pending buffered input and converts
      // it to utf-8. It wouldn't be neccesary if we could use an unbuffered cin
      // (or equivalent) which can read utf-8. Alas, even modern C++ has no
      // portable way of doing so.
      inputstr        = "";
      DWORD numevents = 0;
      do {

        wchar_t wstr[32]  = L"";
        char    mbstr[96] = "";
        DWORD   read;
        // Read the utf-16 console input.
        if (ReadConsole(hcin, wstr, 32, &read, NULL) == 0) { break; }
        // Convert it to utf-8.
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr, read, mbstr,
                                       sizeof(mbstr), nullptr, nullptr);
        // Stick a null terminator at the end, and append it to our input
        // string.
        mbstr[size] = '\0';
        inputstr += mbstr;

        // Get the new number of events waiting.
        GetNumberOfConsoleInputEvents(hcin, &numevents);
      } while (numevents > 0);

      if (inputstr.size() == 0) continue;

      switch (inputstr[0]) {
        // If the input is \n or \r,
        // the user has hit 'enter' to submit their input.
      case '\n':
      case '\r':
        // Convert the input buffer from utf32 to utf8.
        qstr = utf32_to_utf8(*input_buffer);
        // Push it onto the queue.
        cliMutex_g.lock();
        consoleQueue_s.push(move(qstr));
        cliMutex_g.unlock();
        // Clear the buffer and set the cursor to 0.
        if (current_memory_buffer != (int)memory_buffer.size() - 1) {
          if (memory_buffer[memory_buffer.size() - 2] != *input_buffer) {
            memory_buffer.back() = *input_buffer;
            memory_buffer.emplace_back();
          } else {
            memory_buffer.back().clear();
          }
        } else {
          memory_buffer.emplace_back();
        }
        current_memory_buffer = (int)memory_buffer.size() - 1;
        input_buffer          = &memory_buffer[current_memory_buffer];
        input_cursor          = 0;
        break;
        // \x1b is ESC, so we're looking at an esc sequence.
      case '\x1b':
        if (inputstr == "\x1b[A") {  // Up Arrow
          current_memory_buffer = std::max(current_memory_buffer - 1, 0);
          input_buffer          = &memory_buffer[current_memory_buffer];
          input_cursor          = (int)input_buffer->size();
        } else if (inputstr == "\x1b[B") {  // Down Arrow
          current_memory_buffer =
              std::min(current_memory_buffer + 1, (int)memory_buffer.size() - 1);
          input_buffer = &memory_buffer[current_memory_buffer];
          input_cursor = (int)input_buffer->size();
        } else if (inputstr == "\x1b[C") {  // Right Arrow
          input_cursor = std::min(input_cursor + 1, (int)input_buffer->size());
        } else if (inputstr == "\x1b[D") {  // Left Arrow
          input_cursor = std::max(input_cursor - 1, 0);
        } else if (inputstr == "\x1b[H") {  // Home
          current_memory_buffer = (int)memory_buffer.size() - 1;
          input_buffer          = &memory_buffer[current_memory_buffer];
          input_cursor          = (int)input_buffer->size();
        } else if (inputstr == "\x1b[F") {  // End
          input_cursor = (int)input_buffer->size();
        }
        break;
        // 8 is backspace, and 127 is DEL.
        // Either way, it's time to go backwards.
      case 8:
      case 127:
        if (input_buffer->size() > 0 && input_cursor > 0) {
          input_cursor -= 1;
          input_buffer->erase(input_cursor, 1);
        }
        break;
        // If none of the above cases triggered, this is a normal input
        // character.
      default:
        // Convert the raw input from utf8 to utf32.
        temp = utf8_to_utf32(inputstr);
        // Insert the input into the input buffer, and update the cursor.
        input_buffer->insert(input_cursor, temp.c_str());
        input_cursor += (int)temp.size();
        break;
      }

      echostr = utf32_to_utf8(*input_buffer);
      cliMutex_g.lock();
      // Move the cursor to the saved output position.
      std::cout << DECSR;
      // Clear the console after that position.
      std::cout << ED;
      // Print a newline, restore the cursor position, move the cursor up by 1,
      // and then save the cursor position.
      // cout << "\n\x1b[u\x1b[1A\x1b[s";
      // Print the user input.
      // if (myconfig.colorful) {
      std::cout << dbg::USERCOLR << "$> " << dbg::CLEAR << echostr.c_str();
      //}
      // else {
      //	cout << "$> " << echostr.c_str();
      //}
      // Move the cursor so it appears where our input is.
      int cursor_dif = (int)input_buffer->size() - input_cursor;
      if (cursor_dif > 0) {
        char code[8];
        snprintf(code, 8, "\x1b[%iD", cursor_dif);
        std::cout << code;
      }
      cliMutex_g.unlock();
    }
  }

  // This regex ought to find all possible ANSI escape sequences.
  constexpr const char* ANSI_ESC_REGEX_STR("\x1b\\[((?:\\d|;)*)([a-zA-Z])");

  /// Removes ANSI escape sequences from a string.
  void stripAnsi(std::string& str) {
    std::regex r(ANSI_ESC_REGEX_STR);
    str = std::regex_replace(str, r, "");
  }

  /// Creates a copy of a string with ANSI escape sequences removed.
  std::string strippedAnsi(std::string_view str) {
    std::regex r(ANSI_ESC_REGEX_STR);
    return std::regex_replace(std::string(str), r, "");
  }


} // namespace <anon>

namespace cli {
  bool initialize() {
    // Open the log file.
    std::filesystem::path logpath = wc::getUserPath() / LOG_FILENAME;
    logFile_s.open(logpath, std::ios::out);
    

    //	AllocConsole()
    //	freopen("CONIN$", "r", stdin);
    //	freopen("CONOUT$", "w", stdout);
    //	freopen("CONOUT$", "w", stderr);

    // We want both input and output to be UTF8.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // Get the handle for stdout and stdin.
    // TODO: Make sure these actually point to a terminal.
    hcout = GetStdHandle(STD_OUTPUT_HANDLE);
    hcin  = GetStdHandle(STD_INPUT_HANDLE);
    // We want to enable virtual terminal processing for stdout.
    DWORD mode = 0;
    GetConsoleMode(hcout, &mode);
    original_hcout_mode = mode;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hcout, mode);
    // for stdin, we want to enable virtual terminal input,
    // and disable line input and echoing.
    GetConsoleMode(hcin, &mode);
    original_hcin_mode = mode;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    mode ^= ENABLE_LINE_INPUT;
    mode ^= ENABLE_ECHO_INPUT;
    mode ^= ENABLE_PROCESSED_INPUT;
    SetConsoleMode(hcin, mode);
    FlushConsoleInputBuffer(hcin);

    // Save the initial cursor position.
    std::cout << DECSC;

    // Create the input thread, then wake it up.
    cinThread_s = std::thread(terminalInputThread);
    std::cout << WAKEUP;

    initialized_s = true;

    // If we tried to send messages before the console was initialized,
    // we should send them out now.
    //while (!preinitQueue_s.empty()) {
    //  auto entry = preinitQueue_s.front();
    //  preinitQueue_s.pop();
    //  print(std::get<0>(entry), std::get<1>(entry));
    //}

    if (!logFile_s.is_open()) {
      dbg::errmore("Failed to open debug log file for writing.");
    } else {
      dbg::infomore(fmt::format("Output messages will be saved to \"{}\".",
                            logpath.string()));
    }

    return true;
  }

  void shutdown() {
    if (!initialized_s) return;
    // Indicate that the input thread should stop looping.
    inThreadRunning_s = false;
    // Forces an input to be placed on the terminal input stream.
    // This will wake up the thread which is waiting on stdin.
    std::cout << WAKEUP;
    // Wait until the thread is finished before continuing.
    cinThread_s.join();

    SetConsoleMode(hcout, original_hcout_mode);
    SetConsoleMode(hcin, original_hcin_mode);
  }

  void print(dbg::MessageSeverity severity, std::string_view message, bool endl) {
    // If the console hasn't been initialized yet, initialize it.
    if (!initialized_s) {
      initialize();
    }

    // Check for repeated messages so we don't spam the console.
    if ((lastMessage_s == message) && (severity != dbg::MessageSeverity::User))
      return;
    else
      lastMessage_s = message;

    if (myConfig.makeConsole && !!(severity & myConfig.stdoutFilter)) {
      // Try to lock the mutex.
      // This will fail if it was locked during a call to `info`, `error`, etc.
      // In that case we don't need to do anything because we're already synchronized.
      bool wasLocked = cliMutex_g.try_lock();
      // Restore the cursor position to where we last output.
      std::cout << DECSR;
      // Clear the console after the current position.
      std::cout << ED;
      // Print the message.
      std::cout << message;
      if (endl) { std::cout << std::endl; }
      //std::cout << dbg::CLEAR;
      // Save the cursor position.
      std::cout << DECSC;
      // Alert the input thread to update its display.
      std::cout << WAKEUP;
      // Iff we locked the mutex ourselves, unlock it here.
      if (wasLocked) cliMutex_g.unlock();
    }

    if (logFile_s.is_open() && !!(severity & myConfig.logfileFilter)) {
      logFile_s << strippedAnsi(message);
      if (endl) { logFile_s << std::endl; }
    }
  }

  bool popInput(std::string& out) {
    std::lock_guard<std::mutex> lock(cliMutex_g);
    if (consoleQueue_s.empty()) {
      return false;
    } else {
      out = std::move(consoleQueue_s.front());
      consoleQueue_s.pop();
      dbg::user(out);
      return true;
    }
  }
}

#endif // PLATFORM_WIN32