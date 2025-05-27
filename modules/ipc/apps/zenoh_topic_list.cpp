//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <tuple>
#include <utility>

#include <fmt/base.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/topic_database.h"
#include "hephaestus/ipc/topic_filter.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

namespace {

[[nodiscard]] auto getPrintEndpointInfoCallback(heph::ipc::zenoh::SessionPtr session, bool print_topic_info)
    -> heph::ipc::zenoh::EndpointDiscovery::Callback {
  if (!print_topic_info) {
    return &heph::ipc::zenoh::printEndpointInfo;
  }

  std::shared_ptr<heph::ipc::ITopicDatabase> zenoh_topic_db =
      heph::ipc::createZenohTopicDatabase(std::move(session));
  return [zenoh_topic_db = std::move(zenoh_topic_db)](const heph::ipc::zenoh::EndpointInfo& info) mutable {
    heph::ipc::zenoh::printEndpointInfo(info);
    if (info.status == heph::ipc::zenoh::EndpointInfo::Status::DROPPED) {
      return;  // No type info for dropped endpoints.
    }

    switch (info.type) {
      case heph::ipc::zenoh::EndpointType::PUBLISHER:
      case heph::ipc::zenoh::EndpointType::SUBSCRIBER: {
        const auto type_info = zenoh_topic_db->getTypeInfo(info.topic);
        if (type_info) {
          fmt::println("\ttype\t'{}'", type_info->name);
        } else {
          fmt::println("\ttype\tNOT AVAILABLE");
        }
        break;
      }
      case heph::ipc::zenoh::EndpointType::SERVICE_SERVER:
      case heph::ipc::zenoh::EndpointType::SERVICE_CLIENT: {
        const auto type_info = zenoh_topic_db->getServiceTypeInfo(info.topic);
        if (type_info) {
          fmt::println("\ttype\trequest:'{}'\treply:'{}'", type_info->request.name, type_info->reply.name);
        } else {
          fmt::println("\ttype\tNOT AVAILABLE");
        }
        break;
      }
      case heph::ipc::zenoh::EndpointType::ACTION_SERVER:
        const auto type_info = zenoh_topic_db->getActionServerTypeInfo(info.topic);
        if (type_info) {
          fmt::println("\ttype\trequest:'{}'\treply:'{}'\tstatus:'{}'", type_info->request.name,
                       type_info->reply.name, type_info->status.name);
        } else {
          fmt::println("\ttype\tNOT AVAILABLE");
        }
        break;
    }
  };
}

void getListOfZenohEndpoints(const heph::ipc::zenoh::Session& session,
                             const heph::ipc::TopicFilter& topic_filter) {
  const auto publishers_info = heph::ipc::zenoh::getListOfEndpoints(session, topic_filter);
  std::ranges::for_each(publishers_info.begin(), publishers_info.end(), &heph::ipc::zenoh::printEndpointInfo);
}

void getLiveListOfZenohEndpoints(heph::ipc::zenoh::SessionPtr session, heph::ipc::TopicFilter topic_filter,
                                 bool print_topic_info) {
  const heph::ipc::zenoh::EndpointDiscovery discover{
    session, std::move(topic_filter), getPrintEndpointInfoCallback(std::move(session), print_topic_info)
  };

  heph::utils::TerminationBlocker::waitForInterrupt();
}
}  // namespace

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;
  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    auto desc = heph::cli::ProgramDescription("List all the publishers of a topic.");
    heph::ipc::zenoh::appendProgramOption(desc);
    desc.defineFlag("live", 'l', "if set the app will keep running waiting for new publisher to advertise");
    desc.defineFlag("type_info", 'i', "print type info of the topics");
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, _, topic_filter_params] = heph::ipc::zenoh::parseProgramOptions(args);
    topic_filter_params.exclude_prefix = "topic_info";
    auto topic_filter = heph::ipc::TopicFilter::create(topic_filter_params);

    fmt::println("Opening session...");
    auto session = heph::ipc::zenoh::createSession(session_config);

    const auto print_type_info = args.getOption<bool>("type_info");
    if (!args.getOption<bool>("live")) {
      getListOfZenohEndpoints(*session, topic_filter);
    } else {
      getLiveListOfZenohEndpoints(session, std::move(topic_filter), print_type_info);
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
