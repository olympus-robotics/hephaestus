//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributor
//=================================================================================================

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <utility>

#include <fmt/base.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/error_handling/panic.h"
#include "hephaestus/ipc/topic_filter.h"
#include "hephaestus/ipc/zenoh/dynamic_subscriber.h"
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/dynamic_deserializer.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/format/format.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

namespace {
constexpr auto DEFAULT_MAX_ARRAY_LENGTH = 100;

void truncateJsonItem(nlohmann::json& obj, size_t max_length) {
  if (obj.is_object()) {
    for (auto& [key, value] : obj.items()) {  // NOLINT(readability-qualified-auto)
      truncateJsonItem(value, max_length);
    }
  } else {
    if (obj.dump().length() > max_length) {
      obj = "<long item>";
    }
  }
}

void truncateLongItems(std::string& msg_json, bool noarr, size_t max_length) {
  if (noarr && msg_json.size() > max_length) {
    nlohmann::json json_obj = nlohmann::json::parse(msg_json);
    truncateJsonItem(json_obj, max_length);
    msg_json = json_obj.dump();
  }
}
}  // namespace

namespace heph::ipc::zenoh::apps {
class TopicEcho {
public:
  TopicEcho(SessionPtr session, TopicFilterParams topic_filter_params, bool noarr,
            std::size_t max_array_length)
    : noarr_(noarr), max_array_length_(max_array_length) {
    DynamicSubscriberParams params{ .session = std::move(session),
                                    .topics_filter_params = std::move(topic_filter_params),
                                    .init_subscriber_cb =
                                        [this](const auto& topic, const auto& type_info) {
                                          (void)topic;
                                          dynamic_deserializer_.registerSchema(type_info);
                                        },
                                    .subscriber_cb =
                                        [this](const auto& metadata, auto data, const auto& topic_info) {
                                          subscribeCallback(metadata, data, topic_info);
                                        } };

    dynamic_subscriber_ = std::make_unique<DynamicSubscriber>(std::move(params));
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
    panicIf(!type_info, "Topic echo requires the type info to run");
    auto msg_json =
        dynamic_deserializer_.toJson(type_info->name, data);  // NOLINT(bugprone-unchecked-optional-access)
    truncateLongItems(msg_json, noarr_, max_array_length_);
    fmt::println("From: {}. Topic: {}\nSequence: {} | Timestamp: {}\n{}", metadata.sender_id, metadata.topic,
                 metadata.sequence_id,
                 utils::format::toString(std::chrono::time_point<std::chrono::system_clock>(
                     std::chrono::duration_cast<std::chrono::system_clock::duration>(metadata.timestamp))),
                 msg_json);
  }

private:
  bool noarr_;
  std::size_t max_array_length_;
  serdes::DynamicDeserializer dynamic_deserializer_;
  std::unique_ptr<DynamicSubscriber> dynamic_subscriber_;
};
}  // namespace heph::ipc::zenoh::apps

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    auto desc = heph::cli::ProgramDescription("Echo the data from a topic to the console.");
    heph::ipc::zenoh::appendProgramOption(desc);

    desc.defineFlag("noarr", "Truncate print of long arrays");
    desc.defineOption<std::size_t>(
        "noarr-max-size",
        fmt::format("Maximal length for an array before being truncated if --noarr is used (Default: {}).",
                    DEFAULT_MAX_ARRAY_LENGTH),
        DEFAULT_MAX_ARRAY_LENGTH);
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, _, topic_filter_params] = heph::ipc::zenoh::parseProgramOptions(args);
    const auto noarr = args.getOption<bool>("noarr");
    const auto max_array_length = args.getOption<std::size_t>("noarr-max-size");

    fmt::println("Opening session...");

    auto session = heph::ipc::zenoh::createSession(session_config);

    heph::ipc::zenoh::apps::TopicEcho topic_echo{ std::move(session), topic_filter_params, noarr,
                                                  max_array_length };
    topic_echo.start().wait();

    heph::utils::TerminationBlocker::waitForInterrupt();

    topic_echo.stop().wait();

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
