//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <string>

enum class ExampleType : uint8_t { PUBSUB, SERVICE, ACTION_SERVER };

[[nodiscard]] inline auto getDefaultTopic(const ExampleType type) -> std::string {
  static constexpr auto DEFAULT_PUBSUB_KEY = "hephaestus/ipc/example/zenoh/put";
  static constexpr auto DEFAULT_SERVICE_KEY = "hephaestus/ipc/example/zenoh/service";
  static constexpr auto DEFAULT_ACTION_SERVER_KEY = "hephaestus/ipc/example/zenoh/action_server";

  switch (type) {
    case ExampleType::PUBSUB:
      return DEFAULT_PUBSUB_KEY;
    case ExampleType::SERVICE:
      return DEFAULT_SERVICE_KEY;
    case ExampleType::ACTION_SERVER:
      return DEFAULT_ACTION_SERVER_KEY;
  }

  __builtin_unreachable();  // TODO(C++23): replace with std::unreachable.
}
