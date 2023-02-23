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
#ifndef WC_TOOLS_RESULT_H
#define WC_TOOLS_RESULT_H

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

    inline bool isSuccess() const { return status_ == Status::success; }
    inline bool isWarning() const { return status_ == Status::warning; }
    inline bool isError() const { return status_ == Status::error; }
    inline bool hasMsg() const { return msg_ != ""; }
    inline const std::string& msg() const { return msg_; }

  protected:
    Status      status_ = Status::unknown;
    std::string msg_    = "";
  };

  /// A result which may or may not contain a value.
  template <typename T>
  class Value: public Empty {
  public:
    Value() = default;
    Value(Status status, const std::string_view msg, const T& val):
        Empty(status, msg), val_(val) {}
    Value(const Empty& empty): Empty(empty), val_(std::nullopt) {}

    inline bool     hasVal() const { return val_.has_value(); }
    inline const T& getVal() const { return val_.value(); }
    inline T&       getVal() { return val_.value(); }

    inline          operator bool() const { return (bool)val_; }
    inline const T& operator*() const { return *val_; }
    inline T&       operator*() { return *val_; }
    inline const T* operator->() const { return &*val_; }
    inline T*       operator->() { return &*val_; }

  protected:
    std::optional<T> val_ = std::nullopt;
  };

  /// @return a successful result with a value.
  template <typename T>
  inline auto success(const T& val) {
    return Value<T>(Status::success, "", val);
  }
  /// @return a successful result with no value.
  inline auto success() { return Empty(Status::success, ""); }

  /// @return a warning result with a value and a message.
  template <typename T>
  inline auto warning(const std::string_view msg, const T& val) {
    return Value<T>(Status::warning, msg, val);
  }
  /// @return a warning result with only a message.
  inline auto warning(const std::string_view msg = "") {
    return Empty(Status::warning, msg);
  }

  /// @return an error result with a message.
  inline auto error(const std::string_view msg = "") {
    return Empty(Status::error, msg);
  }

}  // namespace wc::Result

#endif  // WC_TOOLS_RESULT_H