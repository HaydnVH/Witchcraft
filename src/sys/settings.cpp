/******************************************************************************
 * settings.cpp
 * Part of the Witchcraft engine by Haydn V. Harach
 * https://github.com/HaydnVH/Witchcraft
 * (C) Haydn V. Harach 2023 - present
 * Last modified February 2023
 * ---------------------------------------------------------------------------
 * Provides access to settings.json which contains per-user settings.
 *****************************************************************************/
#include "settings.h"

#include "appconfig.h"
#include "paths.h"
#include "tools/strtoken.h"

#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <sstream>

namespace rj  = rapidjson;
namespace sfs = std::filesystem;

namespace {
  rj::Document doc_s;
  bool         initialized_s = false;
  bool         modified_s    = false;
}  // namespace

void wc::settings::init() {
  sfs::path     filePath = wc::getUserPath() / SETTINGS_FILENAME;
  std::ifstream inFile(filePath, std::ios::in | std::ios::binary);
  if (!inFile.is_open()) {
    // The settings file doesn't exist, so let's create an empty document to
    // write to.
    doc_s.SetObject();
    modified_s = true;
  } else {
    std::stringstream ss;
    ss << inFile.rdbuf();
    doc_s.Parse(ss.str().c_str());
    if (doc_s.HasParseError() || !doc_s.IsObject()) {
      doc_s.SetObject();
      modified_s = true;
    }
  }

  inFile.close();
  initialized_s = true;
}

void wc::settings::shutdown() {
  if (modified_s) {
    sfs::path     filePath = wc::getUserPath() / SETTINGS_FILENAME;
    std::ofstream outFile(filePath, std::ios::out | std::ios::binary);
    if (outFile.is_open()) {
      rj::StringBuffer buffer;
      rj::PrettyWriter writer(buffer);
      doc_s.Accept(writer);
      outFile << buffer.GetString();
    }
  }
}

bool wc::settings::isInitialized() { return initialized_s; }

rj::Value* followPath(const std::string_view path, bool mayCreate) {
  rj::Value* current = nullptr;
  for (auto& it : Tokenizer(path, "", ".")) {
    std::string token(it);
    if (current) {
      if (current->HasMember(token.c_str())) {
        current = &((*current)[token.c_str()]);
        if (!current->IsObject()) return nullptr;
      } else {
        if (mayCreate) {
          current->AddMember(
              rj::Value(token.c_str(), doc_s.GetAllocator()).Move(),
              rj::Value(rj::kObjectType).Move(), doc_s.GetAllocator());
          current = &((*current)[token.c_str()]);
        } else
          return nullptr;
      }
    } else {
      if (doc_s.HasMember(token.c_str())) {
        current = &doc_s[token.c_str()];
        if (!current->IsObject()) return nullptr;
      } else {
        if (mayCreate) {
          doc_s.AddMember(rj::Value(token.c_str(), doc_s.GetAllocator()).Move(),
                          rj::Value(rj::kObjectType).Move(),
                          doc_s.GetAllocator());
          current = &doc_s[token.c_str()];
        } else
          return nullptr;
      }
    }
  }
  return current;
}

bool wc::settings::exists(const std::string_view path) {
  return (followPath(path, false) != nullptr);
}

template <>
bool wc::settings::read<bool>(const std::string_view path, bool& outVal) {
  rj::Value* object = followPath(path, false);
  if (!object) return false;
  if (!object->IsBool()) return false;
  outVal = object->GetBool();
  return true;
}

template <>
bool wc::settings::read<int>(const std::string_view path, int& outVal) {
  rj::Value* object = followPath(path, false);
  if (!object) return false;
  if (!object->IsInt()) return false;
  outVal = object->GetInt();
  return true;
}

template <>
bool wc::settings::read<float>(const std::string_view path, float& outVal) {
  rj::Value* object = followPath(path, false);
  if (!object) return false;
  if (!object->IsFloat()) return false;
  outVal = object->GetFloat();
  return true;
}

template <>
bool wc::settings::read<std::string>(const std::string_view path,
                                     std::string&           outVal) {
  rj::Value* object = followPath(path, false);
  if (!object) return false;
  if (!object->IsString()) return false;
  outVal = object->GetString();
  return true;
}

bool chopTail(const std::string_view path, std::string_view& outTailless,
              std::string_view& outTail) {
  auto pos = path.find_last_of(".");
  if (pos == std::string::npos) return false;
  outTailless = std::string_view(&path[0], pos);
  outTail     = std::string_view(&path[pos + 1], path.size() - (pos + 1));
  return true;
}

template <>
void wc::settings::write<bool>(const std::string_view path, const bool& val) {
  std::string_view tailless, tail;
  chopTail(path, tailless, tail);
  rj::Value* object = followPath(tailless, true);
  if (!object) return;
  while (object->HasMember(tail.data())) { object->RemoveMember(tail.data()); }
  object->AddMember(rj::Value(tail.data(), doc_s.GetAllocator()).Move(), val,
                    doc_s.GetAllocator());
  modified_s = true;
}

template <>
void wc::settings::write<int>(const std::string_view path, const int& val) {
  std::string_view tailless, tail;
  chopTail(path, tailless, tail);
  rj::Value* object = followPath(tailless, true);
  if (!object) return;
  while (object->HasMember(tail.data())) { object->RemoveMember(tail.data()); }
  object->AddMember(rj::Value(tail.data(), doc_s.GetAllocator()).Move(), val,
                    doc_s.GetAllocator());
  modified_s = true;
}

template <>
void wc::settings::write<float>(const std::string_view path, const float& val) {
  std::string_view tailless, tail;
  chopTail(path, tailless, tail);
  rj::Value* object = followPath(tailless, true);
  if (!object) return;
  while (object->HasMember(tail.data())) { object->RemoveMember(tail.data()); }
  object->AddMember(rj::Value(tail.data(), doc_s.GetAllocator()).Move(), val,
                    doc_s.GetAllocator());
  modified_s = true;
}

template <>
void wc::settings::write<std::string_view>(const std::string_view  path,
                                           const std::string_view& val) {
  std::string_view tailless, tail;
  chopTail(path, tailless, tail);
  rj::Value* object = followPath(tailless, true);
  if (!object) return;
  while (object->HasMember(tail.data())) { object->RemoveMember(tail.data()); }
  object->AddMember(rj::Value(tail.data(), doc_s.GetAllocator()).Move(),
                    rj::Value(val.data(), doc_s.GetAllocator()).Move(),
                    doc_s.GetAllocator());
  modified_s = true;
}