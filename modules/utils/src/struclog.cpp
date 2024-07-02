//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#include "hephaestus/utils/struclog.h"

#include <filesystem>
#include <numeric>
#include <string>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace heph::utils::struclog {

Log::Log(const std::string& msg, std::source_location location) {
  {
    std::stringstream ss;
    ss << "message" << FIELD_SEPERATOR << std::quoted(msg);
    logging_data_.emplace_back(ss.str());
  }
  {
    std::stringstream ss;
    ss << "location" << FIELD_SEPERATOR
       << std::quoted(std::format("{}:{}", std::filesystem::path{ location.file_name() }.filename().string(),
                                  location.line()));
    logging_data_.emplace_back(ss.str());
  }
}

auto Log::format() const -> std::string {
  setPattern();

  return std::accumulate(std::next(logging_data_.cbegin()), logging_data_.cend(), logging_data_[0],
                         [](const std::string& accumulated, const std::string& next) {
                           return fmt::format("{}{}{}", accumulated, ELEMENT_SEPERATOR, next);
                         });
}

void Log::setPattern() {
  // We could add %@ to print the current file, voer then we would need to use the SPDLOG_INFO macros,
  // which is less nice and less encapsulated (and could result to clashing versions of fmt).
  spdlog::set_pattern("level=%^%l%$ time=%Y-%m-%dT%H:%M:%S%z %v");
}

void init() {
  spdlog::set_level(spdlog::level::trace);
}

void trace(const Log& s) {
  spdlog::trace(s.format());
}
void debug(const Log& s) {
  spdlog::debug(s.format());
}
void info(const Log& s) {
  spdlog::info(s.format());
}
void warn(const Log& s) {
  spdlog::warn(s.format());
}
void error(const Log& s) {
  spdlog::error(s.format());
}
void critical(const Log& s) {
  spdlog::critical(s.format());
}

}  // namespace heph::utils::struclog
