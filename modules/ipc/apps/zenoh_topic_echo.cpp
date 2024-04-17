//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributor
//=================================================================================================

#include <csignal>
#include <cstdlib>
#include <string>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/serdes/dynamic_deserializer.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/utils/exception.h"
#include "zenoh_program_options.h"

namespace {
constexpr auto MAX_ARRAY_LENGTH = 100;

void truncateJsonItem(nlohmann::json& obj, size_t max_length) {
  if (obj.is_object()) {
    for (auto& [key, value] : obj.items()) {
      truncateJsonItem(value, max_length);
    }
  } else {
    if (obj.dump().length() > max_length) {
      obj = "<long item>";
    }
  }
}

void truncateLongItems(std::string& msg_json, bool noarr, size_t max_length) {
  if (noarr && msg_json.size() > MAX_ARRAY_LENGTH) {
    nlohmann::json json_obj = nlohmann::json::parse(msg_json);
    truncateJsonItem(json_obj, max_length);
    msg_json = json_obj.dump();
  }
}

[[nodiscard]] auto getTopicTypeInfo(heph::ipc::zenoh::Session& session,
                                    const std::string& topic) -> heph::serdes::TypeInfo {
  auto service_topic = heph::ipc::getTypeInfoServiceTopic(topic);
  auto response = heph::ipc::zenoh::callService<std::string, std::string>(
      session, heph::ipc::TopicConfig{ service_topic }, "");
  heph::throwExceptionIf<heph::InvalidDataException>(
      response.size() != 1,
      fmt::format("received {} responses for type from service {}", response.size(), service_topic));

  return heph::serdes::TypeInfo::fromJson(response.front().value);
}
}  // namespace

std::atomic_flag stop_flag = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
auto signalHandler(int /*unused*/) -> void {
  stop_flag.test_and_set();
  stop_flag.notify_all();
}

auto main(int argc, const char* argv[]) -> int {
  try {
    (void)signal(SIGINT, signalHandler);
    (void)signal(SIGTERM, signalHandler);

    auto desc = getProgramDescription("Echo the data from a topic to the console.");
    desc.defineFlag("noarr", "Truncate print of long arrays");
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);

    fmt::println("Opening session...");
    fmt::println("Declaring Subscriber on '{}'", topic_config.name);

    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    // TODO: this needs to be done when we receive the first data as the publisher may not be publishing.
    auto type_info = getTopicTypeInfo(*session, topic_config.name);
    heph::serdes::DynamicDeserializer dynamic_deserializer;
    dynamic_deserializer.registerSchema(type_info);

    auto cb = [&dynamic_deserializer, type = type_info.name, noarr = args.getOption<bool>("noarr")](
                  [[maybe_unused]] const heph::ipc::MessageMetadata& metadata,
                  std::span<const std::byte> buffer) mutable {
      auto msg_json = dynamic_deserializer.toJson(type, buffer);
      truncateLongItems(msg_json, noarr, MAX_ARRAY_LENGTH);
      fmt::println("From: {}. Topic: {} - {}", metadata.sender_id, metadata.topic, msg_json);
    };

    auto subscriber =
        heph::ipc::zenoh::Subscriber{ std::move(session), std::move(topic_config), std::move(cb) };
    (void)subscriber;

    stop_flag.wait(false);

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
