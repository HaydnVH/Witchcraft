#ifndef HVH_WC_CONSOLE_H
#define HVH_WC_CONSOLE_H

#include <string>
#include "tools/stringhelper.h"

class _Console {
public:

	static constexpr const char* LOG_FILENAME = "log.txt";
	static constexpr const int SAVED_MESSAGE_SIZE = 256;

	enum PrintlogSeverity
	{
		INFO = 1,
		WARNING,
		ERROR,
		FATAL,
		USER,
		NEVER
	};

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

	inline const char* rgb(unsigned char r, unsigned char g, unsigned char b) {
		static char buffer[24];
		snprintf(buffer, 24, "\x1b[38;2;%u;%u;%um", r, g, b);
		return &buffer[0];
	}

	// Recieve a line of input that the user has entered into the console.
	bool popInput(std::string& out_msg);
	// Push a message onto the queue of crash reports to be displayed in message boxes.
	void pushCrashReport(std::string_view msg);
	// Show each crash report to the user in a message box.
	bool showCrashReports();

	// This version of print does all of the heavy lifting,
	// Everything else is just a shortcut.
	void _print(int severity, std::string_view color, std::string_view msg);

	template <typename... Args>
	inline void print(int severity, std::string_view color, const Args&... args) {
		_print(severity, color, makestr(args...));
	}

	template <typename... Args>
	inline void info(const Args&... args) {
		print(INFO, INFOCOLR, INFOMARK);
		print(INFO, "", args...);
	}

	template <typename... Args>
	inline void infomore(const Args&... args) {
		print(INFO, INFOCOLR, INFOMORE);
		print(INFO, "", args...);
	}

	template <typename... Args>
	inline void warning(const Args&... args) {
		print(WARNING, WARNCOLR, WARNMARK);
		print(INFO, "", args...);
	}

	template <typename... Args>
	inline void warnmore(const Args&... args) {
		print(WARNING, WARNCOLR, WARNMARK);
		print(INFO, "", args...);
	}

	template <typename... Args>
	inline void error(const Args&... args) {
		print(ERROR, ERRCOLR, ERRMARK);
		print(ERROR, "", args...);
	}

	template <typename... Args>
	inline void errmore(const Args&... args) {
		print(ERROR, ERRCOLR, ERRMARK);
		print(ERROR, "", args...);
	}

	template <typename... Args>
	inline void fatal(const Args&... args) {
		std::string msg = makestr(args...);
		pushCrashReport(msg);
		print(ERROR, FATALCOLR, FATALMARK);
		print(FATAL, "", msg);
	}

	template <typename... Args>
	inline void user(const Args&... args) {
		print(USER, USERCOLR, USERMARK);
		print(USER, "", args...);
	}

	template <typename... Args>
	inline void usermore(const Args&... args) {
		print(USER, USERCOLR, USERMORE);
		print(USER, "", args...);
	}

	static _Console& getSingleton() { static _Console instance; return instance; }

private:
	_Console();
	~_Console();

}; // _Console

inline _Console& console() { return _Console::getSingleton(); }

#endif // HVH_WC_CONSOLE_H