//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#include "hephaestus/telemetry/struclog.h"

#include <filesystem>
#include <iomanip>
#include <iterator>
#include <numeric>
#include <source_location>
#include <sstream>
#include <string>

#include <fmt/format.h>

namespace heph::telemetry {

Log::Log(const std::string& msg, std::source_location location) {
  {
    std::stringstream ss;
    ss << "message" << FIELD_SEPERATOR << std::quoted(msg);
    logging_data_.emplace_back(ss.str());
  }
  {
    std::stringstream ss;
    ss << "location" << FIELD_SEPERATOR
       << std::quoted(fmt::format("{}:{}", std::filesystem::path{ location.file_name() }.filename().string(),
                                  location.line()));
    logging_data_.emplace_back(ss.str());
  }
}

auto Log::format() const -> std::string {
  return std::accumulate(std::next(logging_data_.cbegin()), logging_data_.cend(), logging_data_[0],
                         [](const std::string& accumulated, const std::string& next) {
                           return fmt::format("{}{}{}", accumulated, ELEMENT_SEPERATOR, next);
                         });
}

}  // namespace heph::telemetry