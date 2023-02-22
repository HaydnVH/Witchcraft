/******************************************************************************
 * result.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. February 2023 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Provides a useful template system for returning either a value or an error
 * message from a function.
 *****************************************************************************/
#ifndef WC_TOOLS_RESULT_HPP
#define WC_TOOLS_RESULT_HPP

#include <optional>
#include <string>
#include <string_view>

namespace wc::Result {
  enum class Status { unknown, success, warning, error };

  /// An empty result; contains only a status and (possibly) a message.
  class Empty {
  public:
    Empty() = default;
    Empty(Status status, const std::string_view msg):
        status_(status), msg_(msg) {}

    bool               isSuccess() const { return status_ == Status::success; }
    bool               isWarning() const { return status_ == Status::warning; }
    bool               isError() const { return status_ == Status::error; }
    bool               hasMessage() const { return msg_ != ""; }
    const std::string& getMessage() const { return msg_; }

  protected:
    Status      status_ = Status::unknown;
    std::string msg_    = "";
  };

  /// A result which may or may not contain a value.
  template <typename T>
  class Value: public Empty {
  public:
    Value() = default;
    Value(Status status, const T& val, const std::string_view msg):
        Empty(status, msg), val_(val) {}
    Value(const Empty& empty): Empty(empty), val_(std::nullopt) {}

    bool     hasValue() const { return val_.has_value(); }
    const T& getValue() const { return val_.value(); }
    T&       getValue() { return val_.value(); }

  protected:
    std::optional<T> val_ = std::nullopt;
  };

  /// @return a successful result with a value.
  template <typename T>
  static auto success(const T& val) {
    return Value<T>(Status::success, val, "");
  }
  /// @return a successful result with no value.
  static Empty success() { return Empty(Status::success, ""); }

  /// @return a warning result with a value and a message.
  template <typename T>
  static auto warning(const T& val, const std::string_view msg = "") {
    return Value<T>(Status::warning, val, msg);
  }
  /// @return a warning result with only a message.
  static Empty warning(const std::string_view msg = "") {
    return Empty(Status::warning, "");
  }

  /// @return an error result with a message.
  static Empty error(const std::string_view msg = "") {
    return Empty(Status::error, msg);
  }

}  // namespace wc::Result

#endif  // WC_TOOLS_RESULT_HPP