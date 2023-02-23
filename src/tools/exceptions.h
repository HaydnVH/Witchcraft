#ifndef WC_TOOLS_EXCEPTIONS_H
#define WC_TOOLS_EXCEPTIONS_H

#include <filesystem>
#include <fmt/core.h>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace dbg {

  class Exception: public std::exception {
  public:
    Exception(const std::string_view msg,
              std::source_location   src = std::source_location::current()) {
      addMsg(msg, src);
    }
    Exception(const std::string_view msg, Exception&& e,
              std::source_location src = std::source_location::current()) {
      *this = std::move(e);
      addMsg(msg, src);
    }

    auto       begin() { return messages_.rbegin(); }
    const auto begin() const { return messages_.rbegin(); }
    auto       end() { return messages_.rend(); }
    const auto end() const { return messages_.rend(); }
    size_t     size() const { return messages_.size(); }

  private:
    std::vector<std::string> messages_;

    Exception& addMsg(const std::string_view msg, std::source_location src) {
      messages_.push_back(fmt::format(
          "[{}:{} \"{}\"]:\x1b[0m {}",
          std::filesystem::path(src.file_name()).filename().string(),
          src.line(), src.function_name(), msg));
      return *this;
    }
  };

}  // namespace dbg

#endif  // WC_TOOLS_EXCEPTIONS_H