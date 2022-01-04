#ifndef HVH_WC_TOOLS_STRINGHELPER_H
#define HVH_WC_TOOLS_STRINGHELPER_H

#include <string>
#include <sstream>
#include <codecvt>
#include <locale>

template <typename... Args>
inline std::string makestr(const Args&... args) {
	std::stringstream ss;
	(ss << ... << args);
	return ss.str();
}

inline std::wstring utf8_to_utf16(const std::string& in) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t, 1114111UL, std::little_endian>, wchar_t> converter;
	return converter.from_bytes(in);
}

inline std::string utf16_to_utf8(const std::wstring& in) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
	return converter.to_bytes(in);
}

inline std::u32string utf8_to_utf32(const std::string& in) {

	std::wstring_convert<std::codecvt_utf8<char32_t, 1114111UL, std::little_endian>, char32_t> converter;
	return converter.from_bytes(in);
}

inline std::string utf32_to_utf8(const std::u32string& in) {
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
	return converter.to_bytes(in);
}

/* _is_matched_quote is a helper function used by tokenize.
 * It returns true if the spot at `pos` in `str` is a quotation mark which has a matching mark later in the string,
 * or false otherwise.
 * Complexity: O(n).
 */
inline bool _is_matched_quote(std::string_view str, size_t pos, std::string_view quotes) {
	using namespace std;
	if (pos >= str.size() || quotes.find_first_of(str[pos]) == string::npos)
		return false;

	char quotemark = str[pos];
	for (size_t i = pos; i < str.size(); ++i) {
		if (str[i] == quotemark) { return true; }
	}
	return false;
}

/* tokenize
 * This function behaves similarly to 'strtok',
 * but utilizing string_view to avoid modifying the original string.
 * This function also respects quotation marks and will treat a quote within a string as a single token.
 * Paramters:
 * - in: On the first iteration, this should be the string to tokenize.
 *     On subsequent iterations, this should be "" or `string_view()`.
 * - quotes: The list of characters which are considered a quotation mark.  Quotes can be inside quotes if their mark is different.
 *     By default, this is ' and ".
 *     Setting this parameter to an empty string will have it not respect quotes similar to the original `strtok`.
 * - delimiters: The list of characters which are considered a delimiter when splitting strings.
 *     By default, this is all standard whitespace characters.
 * Warning: Undefined behaviour if any character appears in both `quotes` and `delimiters`!
 * Returns:
 *   The token which corresponds to the iteration,
 *   or a newly initialized `string_view` if there's no more string to split apart.
 *   Use data() instead of size() on the returned string to discern between an
 *   empty string (which could be a valid token) and a null string (indicating there are no more tokens).
 * Complexity: O(n)
 */
inline std::string_view tokenize(std::string_view in, std::string_view quotes = "'\"", std::string_view delimiters = " \t\n\v\f\r") {
	using namespace std;
	static string_view str;
	static char quotestate;
	string_view result;
	// If `in` is not an empty string, then this is our first iteration.
	if (in.size() > 0) {
		str = in;
		quotestate = ' ';
	}

	// If we're not inside a quote, clean up the front of the string.
	if (quotestate == ' ') {
		bool empty = true;
		// Scan through the string...
		for (size_t i = 0; i < str.size(); ++i) {
			// Until we hit something that isn't a delimiter.
			if (delimiters.find_first_of(str[i]) == string::npos) {
				// If what we found is a matched quote, we need to go 1 further.
				if (_is_matched_quote(str, i, quotes)) {
					quotestate = str[i];
					++i;
				}
				// Trim the front of the string.
				if (i != 0) { str = string_view(&str[i], str.size() - i); }
				empty = false;
				break;
			}
		}
		// If the string is oops all delimiters, we get rid of that shit.
		if (empty) { str = string_view(); }
	}

	for (size_t i = 0; i < str.size(); ++i) {
		if (quotestate == ' ') {
			// If the current character is a matched quote...
			if (_is_matched_quote(str, i, quotes)) {
				quotestate = str[i];
				result = string_view(&str[0], i);
				str = string_view(&str[i + 1], str.size() - (i + 1));
				return result;
			}
			// If the current character is a delimiter...
			if (delimiters.find_first_of(str[i]) != string::npos) {
				result = string_view(&str[0], i);
				if (i + 1 < str.size()) { str = string_view(&str[i + 1], str.size() - (i + 1)); }
				else { str = string_view(); }
				return result;
			}
		}
		// If the current character is the end of the quote we're in...
		else if (str[i] == quotestate) {
			quotestate = ' ';
			result = string_view(&str[0], i);
			if (i + 1 < str.size()) { str = string_view(&str[i + 1], str.size() - (i + 1)); }
			else { str = string_view(); }
			return result;
		}
	}

	// If we've reached this far, then we're at the end of the string.
	result = str;
	str = string_view();
	quotestate = ' ';
	return result;
}

/* _is_matched_tag is a helper function used by tokenize_tags.
 * It returns true if the spot at `pos` in `str` is a starting tag mark which has a matching end mark later in the string,
 * or false otherwise.
 * Complexity: O(n).
 */
inline bool _is_matched_tag(std::string_view str, size_t pos, std::string_view quotes, char tagbegin, char tagend) {
	using namespace std;
	if (pos >= str.size() || str[pos] != tagbegin)
		return false;

	char quotestate = ' ';
	for (size_t i = pos; i < str.size(); ++i) {
		if (quotestate == ' ') {
			if (str[i] == tagend)
				return true;
			if (_is_matched_quote(str, i, quotes))
				quotestate = str[i];
		}
		else {
			if (str[i] == quotestate)
				quotestate = ' ';
		}
	}
	return false;
}

/* tokenize_tags
 * This function splits a string into tokens according to the presence of tags within the string.
 * Whitespace is not affected.
 * Parameters:
 * - in: On the first iteration, this should be the string to tokenize.
 *     On subsequent iterations, this should be "" or `string_view()`.
 * - quotes: The list of characters which are considered a quotation mark.  Quotes can be inside quotes if their mark is different.
 *     By default, this is ' and ".
 *     Setting this parameter to an empty string will have it not respect quotes similar to the original `strtok`.
 * - delimiters: The list of characters which are considered a delimiter when splitting strings.
 *     By default, this is all standard whitespace characters.
 * Warning: Undefined behaviour if any character appears in both `quotes` and `delimiters`!
 * Returns:
 *   The token which corresponds to the iteration,
 *   or a newly initialized `string_view` if there's no more string to split apart.
 *   Use data() instead of size() on the returned string to discern between an
 *   empty string (which could be a valid token) and a null string (indicating there are no more tokens).
 * Complexity: O(n)
 */
inline std::string_view tokenize_tags(std::string_view in, std::string_view quotes = "'\"", char tagbegin = '<', char tagend = '>') {
	using namespace std;
	static string_view str;
	static bool intag = false;
	char quotestate = ' ';
	string_view result;
	if (in.size() > 0) { str = in; intag = false; }
	if (str.size() == 0) { return string_view(); }

	// Go through the string.
	for (size_t i = 0; i < str.size(); ++i) {
		// If we're not in a quote...
		if (quotestate == ' ') {
			// If the current char is a matched quote mark,
			if (_is_matched_quote(str, i, quotes)) {
				// save it as our quote state and move on.
				quotestate = str[i];
				continue;
			}
			// If we're not in a tag and the current char is a matched tag,
			// OR if we are in a tag and the current char is a tagend,
			if ((!intag && _is_matched_tag(str, i, quotes, tagbegin, tagend)) ||
				(intag && str[i] == tagend))
			{
				// Return this token and save the rest of the string for next time.
				result = string_view(&str[0], i);
				if (i + 1 < str.size()) { str = string_view(&str[i + 1], str.size() - (i + 1)); }
				else str = string_view();
				intag = !intag;
				return result;
			}
		}
		// If we are in a quote, and we've hit a matching end-quote,
		else if (quotestate == str[i]) {
			// Reset the quote state.
			quotestate = ' ';
		}
	}

	// If we get this far, then we've hit the end of the string.
	result = str;
	str = string_view();
	intag = false;
	return result;
}
/*
inline std::vector<std::string_view> splitstr_nondestructive(std::string_view str, char delim = ' ') {
	std::vector<std::string_view> result;
	size_t start = 0, i;
	bool finished_str = false;
	for (i = start; i < str.size(); ++i) {

		if (str[i] == delim) {
			finished_str = true;
		}
		else if (finished_str) {
			result.emplace_back(&str[start], i - start);
			start = i;
			finished_str = false;
		}

	}
	if (i > start) {
		result.emplace_back(&str[start], i - start);
	}
	return result;
}*/

inline void lowercase(char* str) {
	if (str == nullptr) return;
	for (size_t i = 0; str[i] != '\0'; ++i) {
		str[i] = tolower(str[i]);
}	}

inline void lowercase(std::string& str) {
	for (char& c : str) {
		c = tolower(c);
}	}

inline void lowercase_proper(std::string& str) {
	std::locale loc;
	std::u32string wstr = utf8_to_utf32(str);
	for (char32_t& c : wstr) {
		c = std::tolower(c, loc);
	}
	str = utf32_to_utf8(wstr);
}

inline void strip_backslashes(char* str) {
	if (str == nullptr) return;
	for (size_t i = 0; str[i] != '\0'; ++i) {
		if (str[i] == '\\') str[i] = '/';
}	}

inline void strip_backslashes(std::string& str) {
	for (char& c : str) {
		if (c == '\\') c = '/';
}	}

inline void strip_ansi_colors(std::string& str) {
	for (size_t i = 0; str[i] != '\0'; ++i) {
		if (str[i] == '\x1b') {
			for (size_t j = i; str[j] != '\0'; ++j) {
				if (str[j] == 'm') {
					str.erase(i, (j - i)+1);
					--i;
					break;
				}
			}
		}
	}
}

#endif // HVH_WC_TOOLS_STRINGHELPER_H