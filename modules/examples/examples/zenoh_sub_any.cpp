//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributor
//=================================================================================================

#include <chrono>
#include <cstdlib>
#include <format>
#include <thread>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "eolo/base/exception.h"
#include "eolo/examples/types_protobuf/pose.h"
#include "eolo/ipc/zenoh/query.h"
#include "eolo/ipc/zenoh/session.h"
#include "eolo/ipc/zenoh/subscriber.h"
#include "eolo/serdes/dynamic_deserializer.h"
#include "eolo/serdes/type_info.h"
#include "zenoh_program_options.h"

[[nodiscard]] auto getTopicTypeInfo(zenohc::Session& session,
                                    const std::string& topic) -> eolo::serdes::TypeInfo {
  auto service_topic = eolo::ipc::getTypeInfoServiceTopic(topic);
  auto response = eolo::ipc::zenoh::query(session, service_topic, "");
  eolo::throwExceptionIf<eolo::InvalidDataException>(
      response.size() != 1,
      std::format("received {} responses for type from service {}", response.size(), service_topic));

  return eolo::serdes::TypeInfo::fromJson(response.front().value);
}

auto main(int argc, const char* argv[]) -> int {
  try {
    auto desc = getProgramDescription("Periodic publisher example");
    const auto args = std::move(desc).parse(argc, argv);

    auto config = parseArgs(args);

    fmt::println("Opening session...");
    fmt::println("Declaring Subscriber on '{}'", config.topic);

    auto session = eolo::ipc::zenoh::createSession(config);

    // TODO: this needs to be done when we receive the first data as the publisher may not be publishing.
    auto type_info = getTopicTypeInfo(*session, config.topic);
    eolo::serdes::DynamicDeserializer dynamic_deserializer;
    dynamic_deserializer.registerSchema(type_info);

    auto cb = [&dynamic_deserializer, type = type_info.name](const eolo::ipc::MessageMetadata& metadata,
                                                             std::span<const std::byte> buffer) mutable {
      auto msg_json = dynamic_deserializer.toJson(type, buffer);
      fmt::println("From: {}. Topic: {} - {}", metadata.sender_id, metadata.topic, msg_json);
    };

    auto subscriber = eolo::ipc::zenoh::Subscriber{ std::move(session), std::move(config), std::move(cb) };

    (void)subscriber;

    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds{ 1 });
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
