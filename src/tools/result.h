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
#include <source_location>
#include <string>
#include <string_view>
#include <variant>

namespace wc::Result {
  enum class Status { unknown, success, warning, error };

  using Source =
      std::variant<std::source_location, std::string, std::nullopt_t>;

  /// An empty result; contains only a status and (possibly) a message.
  class Empty {
  public:
    Empty() = default;
    Empty(Status status, const std::string_view msg,
          Source context = std::nullopt):
        status_(status),
        msg_(msg), context_(context) {}

    inline bool isSuccess() const { return status_ == Status::success; }
    inline bool isWarning() const { return status_ == Status::warning; }
    inline bool isError() const { return status_ == Status::error; }
    inline bool hasMsg() const { return msg_ != ""; }
    inline const std::string& msg() const { return msg_; }
    inline bool               hasContext() {
      return !std::holds_alternative<std::nullopt_t>(context_);
    }
    inline const Source& context() const { return context_; }

    const Empty&
        setContext(std::source_location src = std::source_location::current()) {
      context_ = src;
      return *this;
    }
    const Empty& setContext(const std::string_view src) {
      context_ = std::string(src);
      return *this;
    }

  protected:
    Status      status_  = Status::unknown;
    std::string msg_     = "";
    Source      context_ = "";
  };

  /// A result which may or may not contain a value.
  template <typename T>
  class Value: public Empty {
  public:
    Value() = default;
    Value(Status status, const std::string_view msg, T&& val):
        Empty(status, msg), val_(std::move(val)) {}
    Value(const Empty& empty): Empty(empty), val_(std::nullopt) {}

    inline bool     hasVal() const { return val_.has_value(); }
    inline const T& val() const { return val_.value(); }
    inline T&       val() { return val_.value(); }

    inline           operator bool() const { return (bool)val_; }
    inline const T*  operator->() const { return val_.operator->(); }
    inline T*        operator->() { return val_.operator->(); }
    inline const T&  operator*() const& { return *val_; }
    inline T&        operator*() & { return *val_; }
    inline const T&& operator*() const&& { return std::move(*val_); }
    inline T&&       operator*() && { return std::move(*val_); }

  protected:
    std::optional<T> val_ = std::nullopt;
  };

  /// @return a successful result with a value.
  template <typename T>
  inline Value<T> success(T& val) {
    return Value<T>(Status::success, "", std::move(val));
  }
  /// @return a successful result with a value.
  template <typename T>
  inline Value<T> success(T&& val) {
    return Value<T>(Status::success, "", std::move(val));
  }
  /// @return a successful result with no value.
  inline Empty success() { return Empty(Status::success, ""); }

  /// @return a warning result with a value and a message.
  template <typename T>
  inline Value<T> warning(const std::string_view msg, T& val) {
    return Value<T>(Status::warning, msg, std::move(val));
  }
  /// @return a warning result with a value and a message.
  template <typename T>
  inline Value<T> warning(const std::string_view msg, T&& val) {
    return Value<T>(Status::warning, msg, std::move(val));
  }
  /// @return a warning result with only a message.
  inline Empty warning(const std::string_view msg = "") {
    return Empty(Status::warning, msg);
  }

  /// @return an error result with a message.
  inline Empty error(const std::string_view msg = "") {
    return Empty(Status::error, msg);
  }

}  // namespace wc::Result

#endif  // WC_TOOLS_RESULT_H