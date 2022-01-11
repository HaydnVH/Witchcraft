/* debug.h
 * This module manages debug output and CLI input.
 * by Haydn V. Harach
 * Created October 2019
 * Edited December 2021
 *
 * All outputs can be written to file, or to the command line, or both.
 * Uses its own thread so CLI inputs can be non-blocking and synchronous.
 * Virtual terminal sequences are used for colors and formatting, documented here:
 * https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
 * These codes should work on other platforms (untested).
 */
#ifndef HVH_WC_DEBUG_H
#define HVH_WC_DEBUG_H

#include <string>
#include "tools/stringhelper.h"

namespace debug {

	static constexpr const char* LOG_FILENAME = "log.txt";
	static constexpr const int SAVED_MESSAGE_SIZE = 256;

	// The severity of a message.
	// Verbosity settings allow the user to ignore a given type of message.
	enum DebuglogSeverityFlags
	{
		INFO = 1 << 0,
		WARNING = 1 << 1,
		ERROR = 1 << 2,
		FATAL = 1 << 3,
		USER = 1 << 4,
		EVERYTHING = INFO | WARNING | ERROR | FATAL | USER
	};

	// These constants are used to prefix messages.
	static constexpr const char* INFOMARK = "INFO: ";
	static constexpr const char* INFOMORE = "  -   ";
	static constexpr const char* INFOCOLR = "\x1b[38;2;0;255;0m";
	static constexpr const char* WARNMARK = "WARN: ";
	static constexpr const char* WARNMORE = "  ~   ";
	static constexpr const char* WARNCOLR = "\x1b[38;2;255;255;0m";
	static constexpr const char* ERRMARK  = "ERROR: ";
	static constexpr const char* ERRMORE  = "  !    ";
	static constexpr const char* ERRCOLR =	"\x1b[38;2;255;0;0m";
	static constexpr const char* FATALMARK ="FATAL ERROR: ";
	static constexpr const char* FATALMORE ="    !!!      ";
	static constexpr const char* FATALCOLR ="\x1b[38;2;0;0;32m\x1b[48;2;255;60;20m";
	static constexpr const char* USERMARK = " > ";
	static constexpr const char* USERMORE = " - ";
	static constexpr const char* USERCOLR = "\x1b[38;2;0;255;255m";

	// These constants are used to control color.
	static constexpr const char* CLEAR =	"\x1b[0m";
	static constexpr const char* UL =		"\x1b[4m";
	static constexpr const char* NOUL =		"\x1b[24m";
	static constexpr const char* BLACK =	"\x1b[30m";
	static constexpr const char* DARKRED =	"\x1b[31m";
	static constexpr const char* DARKGREEN ="\x1b[32m";
	static constexpr const char* BROWN =	"\x1b[33m";
	static constexpr const char* DARKBLUE =	"\x1b[34m";
	static constexpr const char* PURPLE =	"\x1b[35m";
	static constexpr const char* DARKCYAN =	"\x1b[36m";
	static constexpr const char* WHITE =	"\x1b[37m";
	static constexpr const char* GREY =		"\x1b[90m";
	static constexpr const char* RED =		"\x1b[91m";
	static constexpr const char* GREEN =	"\x1b[92m";
	static constexpr const char* YELLOW =	"\x1b[93m";
	static constexpr const char* BLUE =		"\x1b[94m";
	static constexpr const char* MAGENTA =	"\x1b[95m";
	static constexpr const char* CYAN =		"\x1b[96m";
	static constexpr const char* BRIGHT =	"\x1b[97m";

	// Allows a user to create any color they want for their message.
	inline const char* rgb(unsigned char r, unsigned char g, unsigned char b) {
		static char buffer[24];
		snprintf(buffer, 24, "\x1b[38;2;%u;%u;%um", r, g, b);
		return &buffer[0];
	}

	// Initialize the debug log/console.
	// userpath_utf8: The utf-8 encoded path where log.txt should be placed.
	bool init(const char* userpath_utf8);
	void shutdown();

	// Recieve a line of input that the user has entered into the terminal.
	bool popInput(std::string& out_msg);
	// Push a message onto the queue of crash reports to be displayed in message boxes.
	void pushCrashReport(std::string_view msg);
	// Show each crash report to the user in a message box.
	bool showCrashReports();

	// This version of print does all of the heavy lifting,
	// Everything else is just to add flavor.
	void _print(int severity, const std::string& msg);

	// Print a message without decoration.
	template <typename... Args>
	inline void print(int severity, const Args&... args) {
		_print(severity, makestr(args...));
	}

	// Print an 'INFO' message.
	template <typename... Args>
	inline void info(const Args&... args) {
		print(INFO, INFOCOLR, INFOMARK);
		print(INFO, "", args...);
	}

	// Print additional 'INFO' information.
	template <typename... Args>
	inline void infomore(const Args&... args) {
		print(INFO, INFOCOLR, INFOMORE);
		print(INFO, "", args...);
	}

	// Print a 'WARN' message.
	template <typename... Args>
	inline void warning(const Args&... args) {
		print(WARNING, WARNCOLR, WARNMARK);
		print(INFO, "", args...);
	}

	// Print additional 'WARN' information.
	template <typename... Args>
	inline void warnmore(const Args&... args) {
		print(WARNING, WARNCOLR, WARNMORE);
		print(INFO, "", args...);
	}

	// Print an 'ERROR' message.
	template <typename... Args>
	inline void error(const Args&... args) {
		print(ERROR, ERRCOLR, ERRMARK);
		print(ERROR, "", args...);
	}

	// Print additional 'ERROR' information.
	template <typename... Args>
	inline void errmore(const Args&... args) {
		print(ERROR, ERRCOLR, ERRMORE);
		print(ERROR, "", args...);
	}

	// Print a 'FATAL' message.
	// This will be pushed to the crash reports.
	template <typename... Args>
	inline void fatal(const Args&... args) {
		std::string msg = makestr(args...);
		pushCrashReport(msg);
		print(ERROR, FATALCOLR, FATALMARK);
		print(FATAL, "", msg);
	}

	// Print a 'USER' message.
	template <typename... Args>
	inline void user(const Args&... args) {
		print(USER, USERCOLR, USERMARK);
		print(USER, "", args...);
	}

	// Print additional 'USER' information.
	template <typename... Args>
	inline void usermore(const Args&... args) {
		print(USER, USERCOLR, USERMORE);
		print(USER, "", args...);
	}

}; // namespace debug


#endif // HVH_WC_DEBUG_H