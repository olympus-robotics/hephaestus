# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME ipc
  DEPENDS_ON_MODULES cli concurrency containers utils serdes types
  DEPENDS_ON_EXTERNAL_PROJECTS absl fmt nlohmann_json range-v3 zenohc zenohcxx
)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)
find_package(range-v3 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(zenohc REQUIRED)
find_package(zenohcxx REQUIRED)

add_subdirectory(proto)

# library sources
set(SOURCES
    src/program_options.cpp
    src/topic_database.cpp
    src/topic_filter.cpp
    src/zenoh/dynamic_subscriber.cpp
    src/zenoh/liveliness.cpp
    src/zenoh/publisher.cpp
    src/zenoh/raw_publisher.cpp
    src/zenoh/service.cpp
    src/zenoh/scout.cpp
    src/zenoh/raw_subscriber.cpp
    src/zenoh/utils.cpp
    src/zenoh/action_server/action_server.cpp
    src/zenoh/action_server/client_helper.cpp
    src/zenoh/action_server/types.cpp
    src/zenoh/action_server/types_protobuf.cpp
    README.md
    include/hephaestus/ipc/program_options.h
    include/hephaestus/ipc/topic_database.h
    include/hephaestus/ipc/topic_filter.h
    include/hephaestus/ipc/zenoh/dynamic_subscriber.h
    include/hephaestus/ipc/zenoh/liveliness.h
    include/hephaestus/ipc/zenoh/publisher.h
    include/hephaestus/ipc/zenoh/raw_publisher.h
    include/hephaestus/ipc/zenoh/service.h
    include/hephaestus/ipc/zenoh/scout.h
    include/hephaestus/ipc/zenoh/raw_subscriber.h
    include/hephaestus/ipc/zenoh/utils.h
    include/hephaestus/ipc/zenoh/action_server/action_server.h
    include/hephaestus/ipc/zenoh/action_server/client_helper.h
    include/hephaestus/ipc/zenoh/action_server/types.h
    include/hephaestus/ipc/zenoh/action_server/types_protobuf.h
)

# library target
define_module_library(
  NAME ipc
  PUBLIC_LINK_LIBS
    absl::base
    absl::status
    absl::log
    hephaestus::concurrency
    hephaestus::containers
    hephaestus::cli
    hephaestus::utils
    hephaestus::serdes
    hephaestus::types
    hephaestus::action_server_types_proto
    fmt::fmt
    nlohmann_json::nlohmann_json
    range-v3::range-v3
    zenohc::lib
    zenohcxx::zenohc
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(apps)
