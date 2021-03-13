#include "console.h"

#ifdef _WIN32
#include <Windows.h>
#else
#pragma error("console.cpp contains Win32 code which may need to be rewritten.")
#endif

#ifdef PLATFORM_SDL2
#include <SDL.h>
#endif

#include "appconfig.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <queue>
#include <array>
#include <cstdio>
#include <thread>
#include <mutex>
#include <atomic>
using namespace std;

#include "tools/fixedstring.h"
#include "tools/stringhelper.h"
#include "tools/crossplatform.h"
//#include "scripting/luasystem.h"

namespace {

	FILE* logfile = nullptr;
	vector<string> crash_reports;
	queue<string> console_queue;
	queue<tuple<int, string, string>> preinit_queue;
	mutex queue_mutex;

	string last_message = "";

	struct {
		int stdout_sensitivity = 0;
		int logfile_sensitivity = 0;
		int console_sensitivity = 0;
		bool colorful = true;
		bool make_console = true;
		bool allow_cheats = true;
		bool modified = false;
	} myconfig;

	thread console_input_thread;
	mutex console_mutex;
	bool inthread_running = true;

	atomic<bool> update_input_line = true;

#ifdef _WIN32
	HANDLE hcout, hcin;
	DWORD original_hcout_mode, original_hcin_mode;
#endif

	// Saves the cursor position in memory.
	constexpr const char DECSC[] = { '\x1b', '7', '\0' };

	// Restores the cursor position from memory.
	constexpr const char DECSR[] = { '\x1b', '8', '\0' };

	// Moves the cursor to the beginning of the next line.
	constexpr const char* IL = "\x1b[1L";

	// Erases characters after the cursor.
	constexpr const char* ED = "\x1b[0J";

	// Wakes up the input thread if it's blocked on stdin.
	constexpr const char* WAKEUP = "\x1b[6n";

	// The main function for the console input thread.
	void consoleInputThread() {

		// 'memory_buffer' remember all of the lines that we've previously input.
		// It includes the line we're currently on, so it starts with 1 entry.
		vector<u32string> memory_buffer(1);
		// The index of the current memory buffer starts with 0.
		int current_memory_buffer = 0;
		// 'input_buffer' points to which memory buffer we're currently on.
		u32string* input_buffer = &memory_buffer.back();
		u32string temp;
		string qstr = "";
		string echostr = "";
		string inputstr = "";
		int input_cursor = 0;

		while (inthread_running) {

			// This chunk of code reads all of the pending buffered input and converts it to utf-8.
			// It wouldn't be neccesary if we could use an unbuffered cin (or equivalent) which can read utf-8.
			// Alas, even modern C++ has no portable way of doing so.
#ifdef _WIN32
			inputstr = "";
			DWORD numevents = 0;
			do {

				wchar_t wstr[32] = L"";
				char mbstr[96] = "";
				DWORD read;
				// Read the utf-16 console input.
				if (ReadConsole(hcin, wstr, 32, &read, NULL) == 0) { break; }
				// Convert it to utf-8.
				int size = WideCharToMultiByte(CP_UTF8, 0, wstr, read, mbstr, sizeof(mbstr), nullptr, nullptr);
				// Stick a null terminator at the end, and append it to our input string.
				mbstr[size] = '\0';
				inputstr += mbstr;

				// Get the new number of events waiting.
				GetNumberOfConsoleInputEvents(hcin, &numevents);
			} while (numevents > 0);
#endif
			if (inputstr.size() == 0) continue;

			switch (inputstr[0]) {
				// If the input is \n or \r,
				// the user has hit 'enter' to submit their input.
			case '\n':
			case '\r':
				// Convert the input buffer from utf32 to utf8.
				qstr = utf32_to_utf8(*input_buffer);
				// Push it onto the queue.
				queue_mutex.lock();
				console_queue.push(move(qstr));
				queue_mutex.unlock();
				// Clear the buffer and set the cursor to 0.
				if (current_memory_buffer != (int)memory_buffer.size() - 1) {
					if (memory_buffer[memory_buffer.size()-2] != *input_buffer) {
						memory_buffer.back() = *input_buffer;
						memory_buffer.emplace_back();
					}
					else {
						memory_buffer.back().clear();
					}
				}
				else {
					memory_buffer.emplace_back();
				}
				current_memory_buffer = (int)memory_buffer.size() - 1;
				input_buffer = &memory_buffer[current_memory_buffer];
				input_cursor = 0;
				break;
				// \x1b is ESC, so we're looking at an esc sequence.
			case '\x1b':
				if (inputstr == "\x1b[A") { // Up Arrow
					current_memory_buffer = max(current_memory_buffer - 1, 0);
					input_buffer = &memory_buffer[current_memory_buffer];
					input_cursor = (int)input_buffer->size();
				}
				else if (inputstr == "\x1b[B") { // Down Arrow
					current_memory_buffer = min(current_memory_buffer + 1, (int)memory_buffer.size() - 1);
					input_buffer = &memory_buffer[current_memory_buffer];
					input_cursor = (int)input_buffer->size();
				}
				else if (inputstr == "\x1b[C") { // Right Arrow
					input_cursor = min(input_cursor + 1, (int)input_buffer->size());
				}
				else if (inputstr == "\x1b[D") { // Left Arrow
					input_cursor = max(input_cursor - 1, 0);
				}
				else if (inputstr == "\x1b[H") { // Home
					current_memory_buffer = (int)memory_buffer.size() - 1;
					input_buffer = &memory_buffer[current_memory_buffer];
					input_cursor = (int)input_buffer->size();
				}
				else if (inputstr == "\x1b[F") { // End
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
				// If none of the above cases triggered, this is a normal input character.
			default:
				// Convert the raw input from utf8 to utf32.
				temp = utf8_to_utf32(inputstr);
				// Insert the input into the input buffer, and update the cursor.
				input_buffer->insert(input_cursor, temp.c_str());
				input_cursor += (int)temp.size();
				break;
			}

			echostr = utf32_to_utf8(*input_buffer);
			console_mutex.lock();
			// Move the cursor to the saved output position.
			cout << DECSR;
			// Clear the console after that position.
			cout << ED;
			// Print a newline, restore the cursor position, move the cursor up by 1, and then save the cursor position.
			//cout << "\n\x1b[u\x1b[1A\x1b[s";
			// Print the user input.
			if (myconfig.colorful) {
				cout << _Console::USERCOLR << "$> " << _Console::CLEAR << echostr.c_str();
			}
			else {
				cout << "$> " << echostr.c_str();
			}
			// Move the cursor so it appears where our input is.
			int cursor_dif = (int)input_buffer->size() - input_cursor;
			if (cursor_dif > 0) {
				char code[8]; snprintf(code, 8, "\x1b[%iD", cursor_dif);
				cout << code;
			}
			console_mutex.unlock();
		}
	}

	void initLua() {
		/*
		lua_State* L = lua::getLuaState();
		if (!L) return;

		lua_getglobal(L, "CONFIG_CONSOLE_INDEXTABLE");
		if (lua_isnil(L, -1)) return;
		lua_pushcfunction(L, [](lua_State* L) {
			const char* key = luaL_checkstring(L, 2);
			if (strcmp(key, "colorful") == 0) { lua_pushboolean(L, myconfig.colorful); return 1; }
			else if (strcmp(key, "allow_cheats") == 0) { lua_pushboolean(L, myconfig.allow_cheats); return 1; }
			else if (strcmp(key, "make_console") == 0) { lua_pushboolean(L, myconfig.make_console); return 1; }
			else { return luaL_error(L, "'", key, "' is not a config setting in 'console'.\n"); }
			return 0;
		}); lua_setfield(L, -2, "__index");
		lua_pop(L, 1);

		lua_getglobal(L, "CONFIG_CONSOLE_NEWINDEXTABLE");
		if (lua_isnil(L, -1)) return;
		lua_pushcfunction(L, [](lua_State* L) {
			const char* key = luaL_checkstring(L, 2);
			if (strcmp(key, "colorful") == 0) {
				if (lua_isboolean(L, 3)) {
					bool newval = lua_toboolean(L, 3);
					if (newval != myconfig.colorful) {
						myconfig.modified = true;
						myconfig.colorful = newval;
						console::info("Value changed to ", newval ? "true" : "false", ".\n");
					}
					else console::info("Value unchanged.\n");
				}
				else return luaL_error(L, "Expected a boolean.\n");
			}
			else if ((strcmp(key, "allow_cheats") == 0) ||
				(strcmp(key, "make_console") == 0))
			{
				console::warning("This value cannot be modified from the console.\n");
				console::warnmore("You'll have to modify config.json and restart.\n");
			}
			else { return luaL_error(L, "'", key, "' is not a config setting in 'console'.\n"); }
			return 0;
		}); lua_setfield(L, -2, "__newindex");
		lua_pop(L, 1);
		*/
	}

} // namespace <anon>

_Console::_Console() {
	// Open the log file.
	std::filesystem::path logpath = appconfig().user_path / LOG_FILENAME;
	logfile = fopen_w(logpath.c_str());

#ifdef _WIN32

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
	hcin = GetStdHandle(STD_INPUT_HANDLE);
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
#endif

	// Save the initial cursor position.
	cout << DECSC;

	// Create the input thread, then wake it up.
	console_input_thread = thread(consoleInputThread);
	cout << WAKEUP;

	// If we tried to send messages before the console was initialized,
	// we should send them out now.
	while (!preinit_queue.empty()) {
		auto& entry = preinit_queue.front();
		preinit_queue.pop();
		print(get<0>(entry), get<1>(entry).c_str(), get<2>(entry));
	}

	// Initialize lua hooks for the console.
	initLua();
}

_Console::~_Console() {
	if (!logfile) return;
	// Indicate that the input thread should stop looping.
	inthread_running = false;
	// Forces an input to be placed on the console input stream.
	// This will wake up the thread which is waiting on stdin.
	cout << WAKEUP;
	// Wait until the thread is finished before continuing.
	console_input_thread.join();

#ifdef _WIN32
	SetConsoleMode(hcout, original_hcout_mode);
	SetConsoleMode(hcin, original_hcin_mode);
#endif
}

void _Console::_print(int severity, string_view color, string_view msg) {

	// If the console hasn't been initialized yet,
	// we put this call on the preinit queue.
	if (!logfile) {
		preinit_queue.push({severity, color.data(), msg.data()});
		return;
	}

	if (color.size() == 0 && severity != USER) {
		// Check for repeated messages so we don't spam the console.
		if (last_message == msg) return;
		else last_message = msg;
	}

	// TODO: Strip tags from the message being printed.

	if (myconfig.make_console && (severity > myconfig.stdout_sensitivity)) {
		console_mutex.lock();
		// Restore the cursor position to where we last output.
		cout << DECSR;
		// Clear the console after the current position.
		cout << ED;
		// Print the message.
		if (color.size() > 0 && myconfig.colorful) { cout << color; }
		cout << msg;
		cout << CLEAR;
		// Save the cursor position.
		cout << DECSC;
		// Alert the input thread to update its display.
		cout << WAKEUP;

		console_mutex.unlock();
	}

	if (logfile && (severity > myconfig.logfile_sensitivity)) {
		fprintf(logfile, msg.data());
	}
}

bool _Console::popInput(string& result) {
	queue_mutex.lock();
	if (console_queue.empty()) {
		queue_mutex.unlock();
		return false;
	}
	else {
		result = move(console_queue.front());
		console_queue.pop();
		queue_mutex.unlock();
		return true;
	}
}

void _Console::pushCrashReport(string_view msg) {
	crash_reports.push_back(msg.data());
}

#ifdef _WIN32
bool _Console::showCrashReports() {
	for (auto& str : crash_reports) {
		MessageBox(NULL, utf8_to_utf16(str).c_str(), L"Fatal Error", MB_ICONEXCLAMATION | MB_OK);
	}
	return (crash_reports.size() > 0);
}
#endif // _WIN32