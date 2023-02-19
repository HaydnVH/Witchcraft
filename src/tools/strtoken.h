/******************************************************************************
 * strtoken.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. February 2022 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Adds a handful of useful functions for splitting a string up into "tokens".
 * Can respectect quotations and tags.
 * TODO: Replace this with an iterator-like system.
 *****************************************************************************/
#ifndef WC_TOOLS_STRTOKEN_H
#define WC_TOOLS_STRTOKEN_H

#include <optional>
#include <string>
#include <string_view>

class Tokenizer {
public:
  Tokenizer(std::string_view str, std::string_view quotes = "'\"",
            std::string_view delimiters = " \t\n\v\f\r"):
      inputString_(str),
      quotes_(quotes), delimiters_(delimiters) {}

  struct Iterator {
  public:
    Iterator(Tokenizer& parent, std::string_view startingState):
        parent_(parent), str_(startingState) {
      next();
    }

    const std::string_view  operator*() { return result_; }
    const std::string_view* operator->() { return &result_; }

    Iterator& operator++() {
      next();
      return *this;
    }
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
      return lhs.result_ == rhs.result_;
    }
    friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
      return lhs.result_ != rhs.result_;
    }

  private:
    void next() {
      // If we're not inside a quote, clean up the front of the string.
      if (quoteState_ == ' ') {
        bool empty = true;
        // Scan through the string...
        for (size_t i = 0; i < str_.size(); ++i) {
          // Until we hit something that isn't a delimiter.
          if (parent_.delimiters_.find(str_[i]) == std::string::npos) {
            // If what we found is a matched quote, we need to go 1 further.
            if (parent_.isMatchedQuote(str_, i)) {
              quoteState_ = str_[i];
              ++i;
            }
            // Trim the front of the string.
            if (i != 0) { str_ = std::string_view(&str_[i], str_.size() - 1); }
            empty = false;
            break;
          }
        }
        // If the string is oops all delimiters, we get rid of it.
        if (empty) { str_ = std::string_view(); }
      }

      for (size_t i = 0; i < str_.size(); ++i) {
        if (quoteState_ == ' ') {
          // If the current character is a matched quote...
          if (parent_.isMatchedQuote(str_, i)) {
            quoteState_ = str_[i];
            result_     = std::string_view(&str_[0], i);
            str_        = std::string_view(&str_[i + 1], str_.size() - (i + 1));
            return;
          }
          // If the character is a delimiter...
          if (parent_.delimiters_.find(str_[i]) != std::string::npos) {
            result_ = std::string_view(&str_[0], i);
            if (i + 1 < str_.size()) {
              str_ = std::string_view(&str_[i + 1], str_.size() - (i + 1));
            } else {
              str_ = std::string_view();
            }
            return;
          }
        }
        // If the current character is the end of the quote we're in...
        else if (str_[i] == quoteState_) {
          quoteState_ = ' ';
          result_     = std::string_view(&str_[0], i);
          if (i + 1 < str_.size()) {
            str_ = std::string_view(&str_[i + 1], str_.size() - (i + 1));
          } else {
            str_ = std::string_view();
          }
          return;
        }
      }
      // If we've reached this far, then we're at the end of the string.
      result_     = str_;
      str_        = std::string_view();
      quoteState_ = ' ';
      return;
    }

    Tokenizer&       parent_;
    std::string_view str_;
    std::string_view result_;
    char             quoteState_ = ' ';
  };

  Iterator begin() { return Iterator(*this, inputString_); }
  Iterator end() { return Iterator(*this, std::string_view()); }

protected:
  bool isMatchedQuote(std::string_view str, size_t pos) {
    if (pos >= str.size() || quotes_.find(str[pos]) == std::string::npos)
      return false;

    char quotemark = str[pos];
    if (str.find(quotemark, pos + 1) != std::string::npos) return true;
    else
      return false;
  }

  bool isMatchedTag(std::string_view str, size_t pos, char tagbegin,
                    char tagend) {
    if (pos >= str.size() || str[pos] != tagbegin) return false;

    char quotestate = ' ';
    for (size_t i = 0; i < str.size(); ++i) {
      if (quotestate == ' ') {
        if (str[i] == tagend) return true;
        if (isMatchedQuote(str, i)) quotestate = str[i];
      } else if (str[i] == quotestate)
        quotestate = ' ';
    }
  }

  std::string_view inputString_;
  std::string_view quotes_;
  std::string_view delimiters_;
};

namespace impl {
  /* isMatchedQuote is a helper function used by strToken.
   * It returns true if the spot at `pos` in `str` is a quotation mark which has
   * a matching mark later in the string, or false otherwise. Complexity: O(n).
   */
  inline bool isMatchedQuote(std::string_view str, size_t pos,
                             std::string_view quotes) {
    using namespace std;
    if (pos >= str.size() || quotes.find_first_of(str[pos]) == string::npos)
      return false;

    char quotemark = str[pos];
    for (size_t i = pos; i < str.size(); ++i) {
      if (str[i] == quotemark) { return true; }
    }
    return false;
  }

  /* isMatchedTag is a helper function used by strTokenTags.
   * It returns true if the spot at `pos` in `str` is a starting tag mark which
   * has a matching end mark later in the string, or false otherwise.
   * Complexity: O(n).
   */
  inline bool isMatchedTag(std::string_view str, size_t pos,
                           std::string_view quotes, char tagbegin,
                           char tagend) {
    using namespace std;
    if (pos >= str.size() || str[pos] != tagbegin) return false;

    char quotestate = ' ';
    for (size_t i = pos; i < str.size(); ++i) {
      if (quotestate == ' ') {
        if (str[i] == tagend) return true;
        if (isMatchedQuote(str, i, quotes)) quotestate = str[i];
      } else {
        if (str[i] == quotestate) quotestate = ' ';
      }
    }
    return false;
  }
}  // namespace impl

struct StrTokenState {
  std::string_view str   = "";
  char             quote = ' ';
};

/* strToken
 * This function behaves similarly to 'strtok',
 * but utilizing string_view to avoid modifying the original string.
 * This function also respects quotation marks and will treat a quote within a
 * string as a single token. Paramters:
 * - in: On the first iteration, this should be the string to tokenize.
 *     On subsequent iterations, this should be "" or `string_view()`.
 * - quotes: The list of characters which are considered a quotation mark.
 * Quotes can be inside quotes if their mark is different. By default, this is '
 * and ". Setting this parameter to an empty string will have it not respect
 * quotes similar to the original `strtok`.
 * - delimiters: The list of characters which are considered a delimiter when
 * splitting strings. By default, this is all standard whitespace characters.
 * Warning: Undefined behaviour if any character appears in both `quotes` and
 * `delimiters`! Returns: The token which corresponds to the iteration, or a
 * newly initialized `string_view` if there's no more string to split apart. Use
 * data() instead of size() on the returned string to discern between an empty
 * string (which could be a valid token) and a null string (indicating there are
 * no more tokens). Complexity: O(n)
 */
inline std::string_view strToken(std::optional<std::string_view> in,
                                 StrTokenState&                  state,
                                 std::string_view                quotes = "'\"",
                                 std::string_view delimiters = " \t\n\v\f\r") {
  using namespace std;
  string_view result;
  // If `in` is not an empty string, then this is our first iteration.
  if (in.has_value()) {
    state.str   = in.value();
    state.quote = ' ';
  }

  // If we're not inside a quote, clean up the front of the string.
  if (state.quote == ' ') {
    bool empty = true;
    // Scan through the string...
    for (size_t i = 0; i < state.str.size(); ++i) {
      // Until we hit something that isn't a delimiter.
      if (delimiters.find_first_of(state.str[i]) == string::npos) {
        // If what we found is a matched quote, we need to go 1 further.
        if (impl::isMatchedQuote(state.str, i, quotes)) {
          state.quote = state.str[i];
          ++i;
        }
        // Trim the front of the string.
        if (i != 0) {
          state.str = string_view(&state.str[i], state.str.size() - i);
        }
        empty = false;
        break;
      }
    }
    // If the string is oops all delimiters, we get rid of that shit.
    if (empty) { state.str = string_view(); }
  }

  for (size_t i = 0; i < state.str.size(); ++i) {
    if (state.quote == ' ') {
      // If the current character is a matched quote...
      if (impl::isMatchedQuote(state.str, i, quotes)) {
        state.quote = state.str[i];
        result      = string_view(&state.str[0], i);
        state.str = string_view(&state.str[i + 1], state.str.size() - (i + 1));
        return result;
      }
      // If the current character is a delimiter...
      if (delimiters.find_first_of(state.str[i]) != string::npos) {
        result = string_view(&state.str[0], i);
        if (i + 1 < state.str.size()) {
          state.str =
              string_view(&state.str[i + 1], state.str.size() - (i + 1));
        } else {
          state.str = string_view();
        }
        return result;
      }
    }
    // If the current character is the end of the quote we're in...
    else if (state.str[i] == state.quote) {
      state.quote = ' ';
      result      = string_view(&state.str[0], i);
      if (i + 1 < state.str.size()) {
        state.str = string_view(&state.str[i + 1], state.str.size() - (i + 1));
      } else {
        state.str = string_view();
      }
      return result;
    }
  }

  // If we've reached this far, then we're at the end of the string.
  result      = state.str;
  state.str   = string_view();
  state.quote = ' ';
  return result;
}

struct StrTokenTagState {
  std::string_view str   = "";
  bool             intag = false;
  char             quote = ' ';
};

/* strTokenTags
 * This function splits a string into tokens according to the presence of tags
 * within the string. Whitespace is not affected. Parameters:
 * - in: On the first iteration, this should be the string to tokenize.
 *     On subsequent iterations, this should be "" or `string_view()`.
 * - quotes: The list of characters which are considered a quotation mark.
 * Quotes can be inside quotes if their mark is different. By default, this is '
 * and ". Setting this parameter to an empty string will have it not respect
 * quotes similar to the original `strtok`.
 * - delimiters: The list of characters which are considered a delimiter when
 * splitting strings. By default, this is all standard whitespace characters.
 * Warning: Undefined behaviour if any character appears in both `quotes` and
 * `delimiters`! Returns: The token which corresponds to the iteration, or a
 * newly initialized `string_view` if there's no more string to split apart. Use
 * data() instead of size() on the returned string to discern between an empty
 * string (which could be a valid token) and a null string (indicating there are
 * no more tokens). Complexity: O(n)
 */
inline std::string_view strTokenTags(std::optional<std::string_view> in,
                                     StrTokenTagState&               state,
                                     std::string_view quotes = "'\"",
                                     char tagbegin = '<', char tagend = '>') {
  using namespace std;
  char        quotestate = ' ';
  string_view result;
  if (in.has_value()) {
    state.str   = in.value();
    state.intag = false;
  }
  if (state.str.size() == 0) { return string_view(); }

  // Go through the string.
  for (size_t i = 0; i < state.str.size(); ++i) {
    // If we're not in a quote...
    if (quotestate == ' ') {
      // If the current char is a matched quote mark,
      if (impl::isMatchedQuote(state.str, i, quotes)) {
        // save it as our quote state and move on.
        quotestate = state.str[i];
        continue;
      }
      // If we're not in a tag and the current char is a matched tag,
      // OR if we are in a tag and the current char is a tagend,
      if ((!state.intag &&
           impl::isMatchedTag(state.str, i, quotes, tagbegin, tagend)) ||
          (state.intag && state.str[i] == tagend)) {
        // Return this token and save the rest of the string for next time.
        result = string_view(&state.str[0], i);
        if (i + 1 < state.str.size()) {
          state.str =
              string_view(&state.str[i + 1], state.str.size() - (i + 1));
        } else
          state.str = string_view();
        state.intag = !state.intag;
        return result;
      }
    }
    // If we are in a quote, and we've hit a matching end-quote,
    else if (quotestate == state.str[i]) {
      // Reset the quote state.
      quotestate = ' ';
    }
  }

  // If we get this far, then we've hit the end of the string.
  result      = state.str;
  state.str   = string_view();
  state.intag = false;
  return result;
}

#endif  // WC_TOOLS_STRTOKEN