//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributor
//=================================================================================================

#include <csignal>
#include <cstdlib>
#include <string>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/dynamic_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/dynamic_deserializer.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/utils/exception.h"
#include "zenoh_program_options.h"

namespace heph::ipc::apps {
class TopicEcho {
public:
  TopicEcho(zenoh::SessionPtr session, TopicConfig topic_config) : topic_config_(std::move(topic_config)) {
    zenoh::DynamicSubscriberParams params{
      .session = std::move(session),
      .topics_filter_params = { .include_topics_only = {},
                                .prefix = topic_config_.name,
                                .exclude_topics = {} },
      .init_subscriber_cb =
          [this](const auto& topic, const auto& type_info) {
            (void)topic;
            dynamic_deserializer_.registerSchema(type_info);
          },
      .subscriber_cb = [this](const auto& metadata, auto data,
                              const auto& topic_info) { subscribeCallback(metadata, data, topic_info); }
    };

    dynamic_subscriber_ = std::make_unique<zenoh::DynamicSubscriber>(std::move(params));
  }

  [[nodiscard]] auto start() -> std::future<void> {
    return dynamic_subscriber_->start();
  }

  [[nodiscard]] auto stop() -> std::future<void> {
    return dynamic_subscriber_->stop();
  }

private:
  void subscribeCallback(const MessageMetadata& metadata, std::span<const std::byte> data,
                         const std::optional<serdes::TypeInfo>& type_info) {
    throwExceptionIf<InvalidParameterException>(!type_info, "Topic echo requires the type info to run");
    auto msg_json = dynamic_deserializer_.toJson(type_info->name, data);
    fmt::println("From: {}. Topic: {} - {}", metadata.sender_id, metadata.topic, msg_json);
  }

private:
  TopicConfig topic_config_;
  heph::serdes::DynamicDeserializer dynamic_deserializer_;
  std::unique_ptr<zenoh::DynamicSubscriber> dynamic_subscriber_;
};
}  // namespace heph::ipc::apps

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
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);

    fmt::println("Opening session...");

    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    heph::ipc::apps::TopicEcho topic_echo{ std::move(session), topic_config };
    topic_echo.start().wait();

    stop_flag.wait(false);

    topic_echo.stop().wait();

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
