//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributor
//=================================================================================================

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <string>
#include <thread>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>
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

namespace {
volatile std::atomic_bool keep_running(true);  // NOLINT
}  // namespace

auto main(int argc, const char* argv[]) -> int {
  std::ignore = std::signal(SIGINT, [](int signal) {
    if (signal == SIGINT) {
      LOG(INFO) << "SIGINT received. Shutting down...";
      keep_running.store(false);
    }
  });
  try {
    auto desc = getProgramDescription("Periodic publisher example", ExampleType::Pubsub);
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);

    fmt::println("Opening session...");
    fmt::println("Declaring Subscriber on '{}'", topic_config.name);

    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    // TODO: this needs to be done when we receive the first data as the publisher may not be publishing.
    auto type_info = getTopicTypeInfo(*session, topic_config.name);
    heph::serdes::DynamicDeserializer dynamic_deserializer;
    dynamic_deserializer.registerSchema(type_info);

    auto cb = [&dynamic_deserializer, type = type_info.name](const heph::ipc::MessageMetadata& metadata,
                                                             std::span<const std::byte> buffer) mutable {
      auto msg_json = dynamic_deserializer.toJson(type, buffer);
      fmt::println("From: {}. Topic: {} - {}", metadata.sender_id, metadata.topic, msg_json);
    };

    auto subscriber =
        heph::ipc::zenoh::Subscriber{ std::move(session), std::move(topic_config), std::move(cb) };
    (void)subscriber;

    while (keep_running.load()) {
      std::this_thread::sleep_for(std::chrono::seconds{ 1 });
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
