/******************************************************************************
 * result.hpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. February 2023 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Provides a useful template class for returning either a value or an error
 * message from a function.
 *****************************************************************************/
#ifndef WC_TOOLS_RESULT_HPP
#define WC_TOOLS_RESULT_HPP

#include <any>
#include <string>
#include <string_view>

namespace wc {

  namespace impl {
    const std::any nullany {};
  }

  class Result {
  public:
    Result() = default;
    enum class ResultStatus { unknown, success, warning, error };

    /// Return a value successfully.
    static Result success(const std::any& val = impl::nullany) {
      return Result(ResultStatus::success, "", val);
    }
    /// Return a value successfully.
    static Result success(std::any&& val) {
      return Result(ResultStatus::success, "", std::move(val));
    }

    /// Indicates that an error has occured and passes along useful info.
    static Result error(const std::string_view msg) {
      return Result(ResultStatus::error, msg, impl::nullany);
    }

    /// Indicates that a non-critical error has occured,
    /// passing along useful info and a usable object.
    static Result warning(const std::string_view msg,
                          const std::any&        val = impl::nullany) {
      return Result(ResultStatus::warning, msg, val);
    }
    /// Indicates that a non-critical error has occured,
    /// passing along useful info and a usable object.
    static Result warning(const std::string_view msg, std::any&& val) {
      return Result(ResultStatus::warning, msg, std::move(val));
    }

    /// @return true if the result has a message, false otherwise.
    bool hasMessage() { return (message_ != ""); }
    /// @return the message associated with the result.
    const std::string& message() { return message_; }

    /// @return true if the result has a value, false otherwise.
    bool hasValue() { return value_.has_value(); }

    /// @return true if the result's value is compatible with the indicated
    /// type.
    template <typename T>
    bool isValue() {
      return (std::any_cast<T>(&value_) != nullptr);
    }

    /// @return A pointer to the value associated with the result.
    /// If there is no value, or if the value's type is incompatible,
    /// a null pointer is returned insted.
    template <typename T>
    T* value() {
      return std::any_cast<T>(&value_);
    }

    /// @return true if the result is a success.
    bool isSuccess() { return status_ == ResultStatus::success; }
    /// @return true if the result is a warning.
    bool isWarning() { return status_ == ResultStatus::warning; }
    /// @return true if the result is an error.
    bool isError() { return status_ == ResultStatus::error; }

    operator bool() { return isSuccess(); }

  private:
    Result(ResultStatus status, const std::string_view msg,
           const std::any& val):
        status_(status),
        message_(msg), value_(val) {}
    Result(ResultStatus status, const std::string_view msg, std::any&& val):
        status_(status), message_(msg), value_(std::move(val)) {}

    ResultStatus status_  = ResultStatus::unknown;
    std::string  message_ = "";
    std::any     value_   = impl::nullany;
  };
}  // namespace wc

#endif  // WC_TOOLS_RESULT_HPP