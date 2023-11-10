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
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <sstream>

namespace rj  = rapidjson;
namespace sfs = std::filesystem;

wc::SettingsFile::SettingsFile(const std::string_view filename):
    filename_(filename) {
  sfs::path     filePath = wc::getUserPath() / filename_;
  std::ifstream inFile(filePath, std::ios::in | std::ios::binary);
  if (!inFile.is_open()) {
    // The settings file doesn't exist, so let's create an empty document to
    // write to.
    doc_.SetObject();
    modified_ = true;
  } else {
    std::stringstream ss;
    ss << inFile.rdbuf();
    doc_.Parse(ss.str().c_str());
    if (doc_.HasParseError() || !doc_.IsObject()) {
      doc_.SetObject();
      modified_ = true;
    }
  }

  inFile.close();
}

wc::SettingsFile::~SettingsFile() {
  if (modified_) {
    sfs::path     filePath = wc::getUserPath() / filename_;
    std::ofstream outFile(filePath, std::ios::out | std::ios::binary);
    if (outFile.is_open()) {
      rj::StringBuffer buffer;
      rj::PrettyWriter writer(buffer);
      doc_.Accept(writer);
      outFile << buffer.GetString();
    }
  }
}

rj::Value*
    wc::SettingsFile::followPath(const std::string_view path, bool mayCreate) {
  rj::Value* current = nullptr;
  for (auto& it : Tokenizer(path, "", ".")) {
    std::string token(it);
    if (current) {
      if (current->IsObject()) {
        if (current->HasMember(token.c_str())) {
          current = &((*current)[token.c_str()]);
        } else {
          if (mayCreate) {
            current->AddMember(
                rj::Value(token.c_str(), doc_.GetAllocator()).Move(),
                rj::Value(rj::kObjectType).Move(), doc_.GetAllocator());
            current = &((*current)[token.c_str()]);
          } else
            return nullptr;
        }
      } else
        return nullptr;
    } else {
      if (doc_.HasMember(token.c_str())) {
        current = &doc_[token.c_str()];
      } else {
        if (mayCreate) {
          doc_.AddMember(rj::Value(token.c_str(), doc_.GetAllocator()).Move(),
                         rj::Value(rj::kObjectType).Move(),
                         doc_.GetAllocator());
          current = &doc_[token.c_str()];
        } else
          return nullptr;
      }
    }
  }
  return current;
}

bool wc::SettingsFile::exists(const std::string_view path) {
  return (followPath(path, false) != nullptr);
}

template <>
bool wc::SettingsFile::read<bool>(const std::string_view path, bool& outVal) {
  rj::Value* object = followPath(path, false);
  if (!object)
    return false;
  if (!object->IsBool())
    return false;
  outVal = object->GetBool();
  return true;
}

template <>
bool wc::SettingsFile::read<int>(const std::string_view path, int& outVal) {
  rj::Value* object = followPath(path, false);
  if (!object)
    return false;
  if (!object->IsInt())
    return false;
  outVal = object->GetInt();
  return true;
}

template <>
bool wc::SettingsFile::read<float>(const std::string_view path, float& outVal) {
  rj::Value* object = followPath(path, false);
  if (!object)
    return false;
  if (!object->IsFloat())
    return false;
  outVal = object->GetFloat();
  return true;
}

template <>
bool wc::SettingsFile::read<std::string>(const std::string_view path,
                                         std::string&           outVal) {
  rj::Value* object = followPath(path, false);
  if (!object)
    return false;
  if (!object->IsString())
    return false;
  outVal = object->GetString();
  return true;
}

namespace {
  bool chopTail(const std::string_view path, std::string_view& outTailless,
                std::string_view& outTail) {
    auto pos = path.find_last_of(".");
    if (pos == std::string::npos)
      return false;
    outTailless = std::string_view(&path[0], pos);
    outTail     = std::string_view(&path[pos + 1], path.size() - (pos + 1));
    return true;
  }
}  // namespace

template <>
void wc::SettingsFile::write<bool>(const std::string_view path,
                                   const bool&            val) {
  std::string_view tailless, tail;
  chopTail(path, tailless, tail);
  rj::Value* object = followPath(tailless, true);
  if (!object)
    return;
  while (object->HasMember(tail.data())) {
    object->RemoveMember(tail.data());
  }
  object->AddMember(rj::Value(tail.data(), doc_.GetAllocator()).Move(), val,
                    doc_.GetAllocator());
  modified_ = true;
}

template <>
void wc::SettingsFile::write<int>(const std::string_view path, const int& val) {
  std::string_view tailless, tail;
  chopTail(path, tailless, tail);
  rj::Value* object = followPath(tailless, true);
  if (!object)
    return;
  while (object->HasMember(tail.data())) {
    object->RemoveMember(tail.data());
  }
  object->AddMember(rj::Value(tail.data(), doc_.GetAllocator()).Move(), val,
                    doc_.GetAllocator());
  modified_ = true;
}

template <>
void wc::SettingsFile::write<float>(const std::string_view path,
                                    const float&           val) {
  std::string_view tailless, tail;
  chopTail(path, tailless, tail);
  rj::Value* object = followPath(tailless, true);
  if (!object)
    return;
  while (object->HasMember(tail.data())) {
    object->RemoveMember(tail.data());
  }
  object->AddMember(rj::Value(tail.data(), doc_.GetAllocator()).Move(), val,
                    doc_.GetAllocator());
  modified_ = true;
}

template <>
void wc::SettingsFile::write<std::string>(const std::string_view path,
                                          const std::string&     val) {
  std::string_view tailless, tail;
  chopTail(path, tailless, tail);
  rj::Value* object = followPath(tailless, true);
  if (!object)
    return;
  while (object->HasMember(tail.data())) {
    object->RemoveMember(tail.data());
  }
  object->AddMember(rj::Value(tail.data(), doc_.GetAllocator()).Move(),
                    rj::Value(val.data(), doc_.GetAllocator()).Move(),
                    doc_.GetAllocator());
  modified_ = true;
}