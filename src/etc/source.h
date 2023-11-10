/******************************************************************************
 * source.h
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified September 2023
 * ---------------------------------------------------------------------------
 * Source is used in various places to define where a message or error came
 * from. 'source_location' is often the default and used for C++ code,
 * but a string can be used instead if a message comes from a script.
 *****************************************************************************/
#ifndef WC_ETC_SOURCE_H
#define WC_ETC_SOURCE_H

#include <optional>
#include <source_location>
#include <string>
#include <variant>

namespace wc {
namespace etc {

  using Source = std::variant<std::nullopt_t, std::source_location, std::string>;

}} // namespace wc::etc

#endif // WC_ETC_SOURCE_H