/******************************************************************************
 * result.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified September 2023
 * ---------------------------------------------------------------------------
 * Provides a useful template system for returning either a value or an error
 * message from a function.
 *****************************************************************************/
#ifndef WC_ETC_RESULT_H
#define WC_ETC_RESULT_H

#include <string_view>
#include <type_traits>
#include "source.h"

namespace wc {
namespace etc {

  struct NullResultT {};
  inline constexpr NullResultT nullResult;

  template <typename T>
  class Result {
  public:
    // Default Constructor
    constexpr Result() = default;
    // Copy Constructor
    template <typename U = T> requires std::is_convertible_v<U, T>
    constexpr Result(const Result<U>& other):
      value_(other.value_), message_(other.message_), source_(other.source_) {}
    // Move Constructor
    template <typename U = T> requires std::is_nothrow_convertible_v<U, T>
    constexpr Result(Result<U>&& other):
      value_(std::move(other.value_)), message_(std::move(other.message_)), source(other.source_) {}
    // Copy Constructor (null other)
    constexpr Result(const Result<NullResultT>& other):
      value_(std::nullopt), message_(other.message_), source(other.source_) {}
    // Move Constructor (null other)
    constexpr Result(Result<NullResultT>&& other):
      value_(std::nullopt), message_(std::move(other.message_)), source(other.source_) {}
    // Initialization constructor
    template <typename U = T> requires std::is_convertible_v<U, T>
    constexpr Result(U&& value, std::optional<std::string_view> message, Source source):
        value_(std::forward<U>(value)), message_(message), source_(source) {}
    // Initializtion constructor (null value)
    constexpr Result(std::nullopt_t, std::optional<std::string_view> message, Source source):
        value_(std::nullopt), message_(message), source_(source) {}

    // Operator bool returns whether a value exists, not whether the operation was successful.
    constexpr operator bool() const { return value_; }

    constexpr T&       operator*() & { return *value_; }
    constexpr const T& operator*() const& { return *value_; }
    constexpr T*       operator&() { return &value_; }
    constexpr const T* operator&() const { return &value_; }

    constexpr bool isSuccess() const { return value_ && !message_; }
    constexpr bool isWarning() const { return value_ && message_; }
    constexpr bool isFailure() const { return !value_ && message_; }

    constexpr T&        value() & { return value_.value(); }
    constexpr const T&  value() const& { return value_.value(); }
    constexpr T&&       value() && { return std::move(value_.value()); }
    constexpr const T&& value() const&& { return std::move(value_.value()); }
    constexpr bool      hasValue() const { return value_.has_value(); }
    template <typename U = T> requires std::is_convertible_v<U,T>
    constexpr T& valueOr(const U& other) { return value_ ? value_.value() : other; }

    constexpr const std::string& message() { return message_ ? message_.value() : ""; }
    constexpr bool               hasMessage() { return message_.has_value(); }

    constexpr const Source& source() { return source_; }

  private:
    std::optional<T>           value_ {std::nullopt};
    std::optional<std::string> message_ {std::nullopt};
    Source                     source_ {std::nullopt};
  };

  /// Creates a Result with a success state (it has a value and no message).
  /// Usage: `return wc::etc::success(val);"`
  template <typename T>
  constexpr inline Result<T>
    success(T&& value, Source source = std::source_location::current()) {
    return Result<T>(std::forward<T>(value), std::nullopt, source);
  }

  /// Creates a Result with a warning state (it has both a value and message).
  /// Usage: `return wc::etc::warning(val, "something happened?");`
  template <typename T>
  constexpr inline Result<T>
    warning(T&& value, std::string_view message, Source source = std::source_location::current()) {
    return Result<T>(std::forward<T>(value), message, source);
  }

  /// Creates a Result with a failure state (it has a message and no value).
  /// Usage: `return wc::etc::failure("something bad happened!");`
  constexpr inline Result<NullResultT>
    failure(std::string_view message, Source source = std::source_location::current()) {
    return Result<NullResultT>(std::nullopt, message, source);
  }

}} // namespace wc::etc

#endif // WC_ETC_RESULT_H