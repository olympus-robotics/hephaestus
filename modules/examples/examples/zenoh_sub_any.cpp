//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributor
//=================================================================================================

#include <chrono>
#include <cstdlib>
#include <format>
#include <thread>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/util/json_util.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "eolo/base/exception.h"
#include "eolo/examples/types/pose.h"
#include "eolo/examples/types_protobuf/pose.h"
#include "eolo/ipc/subscriber.h"
#include "eolo/ipc/zenoh/query.h"
#include "eolo/ipc/zenoh/session.h"
#include "eolo/ipc/zenoh/subscriber.h"
#include "eolo/serdes/type_info.h"
#include "zenoh_program_options.h"

[[nodiscard]] auto getTopicTypeInfo(zenohc::Session& session,
                                    const std::string& topic) -> eolo::serdes::TypeInfo {
  auto service_topic = eolo::ipc::getTypeInfoServiceTopic(topic);
  auto response = eolo::ipc::zenoh::query(session, service_topic, "");
  eolo::throwExceptionIf<eolo::InvalidDataException>(
      response.size() != 1,
      std::format("received {} responses for type from service {}", response.size(), service_topic));

  return eolo::serdes::fromJson(response.front().value);
}

void loadSchema(const eolo::serdes::TypeInfo& type_info,
                google::protobuf::SimpleDescriptorDatabase& proto_db) {
  google::protobuf::FileDescriptorSet fd_set;
  auto res = fd_set.ParseFromArray(type_info.schema.data(), static_cast<int>(type_info.schema.size()));
  eolo::throwExceptionIf<eolo::InvalidDataException>(
      !res, std::format("failed to parse schema for type: {}", type_info.name));

  google::protobuf::FileDescriptorProto unused;
  for (int i = 0; i < fd_set.file_size(); ++i) {
    const auto& file = fd_set.file(i);
    if (!proto_db.FindFileByName(file.name(), &unused)) {
      res = proto_db.Add(file);
      eolo::throwExceptionIf<eolo::InvalidDataException>(
          !res, std::format("failed to add definition to proto DB: {}", file.name()));
    }
  }
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
    google::protobuf::SimpleDescriptorDatabase proto_db;
    google::protobuf::DescriptorPool proto_pool(&proto_db);
    google::protobuf::DynamicMessageFactory proto_factory(&proto_pool);

    loadSchema(type_info, proto_db);
    const auto* descriptor = proto_pool.FindMessageTypeByName(type_info.name);

    auto cb = [descriptor, &proto_factory](const eolo::ipc::MessageMetadata& metadata,
                                           std::span<const std::byte> buffer) mutable {
      google::protobuf::Message* message = proto_factory.GetPrototype(descriptor)->New();
      const auto res = message->ParseFromArray(buffer.data(), static_cast<int>(buffer.size()));
      eolo::throwExceptionIf<eolo::InvalidDataException>(!res, std::format("failed to parse message"));

      std::vector<const google::protobuf::FieldDescriptor*> fields;
      message->GetReflection()->ListFields(*message, &fields);

      std::string msg_json;
      google::protobuf::util::JsonPrintOptions options;
      options.always_print_primitive_fields = true;
      auto status = google::protobuf::util::MessageToJsonString(*message, &msg_json);
      eolo::throwExceptionIf<eolo::InvalidDataException>(
          !status.ok(), std::format("failed to convert proto message to json: {}", status.message()));
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
