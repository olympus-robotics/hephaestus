#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

declare_module(
        NAME ws_bridge
        DEPENDS_ON_MODULES ipc cli concurrency containers utils serdes telemetry
        DEPENDS_ON_EXTERNAL_PROJECTS  absl fmt nlohmann_json zenohc zenohcxx ws_protocol)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(zenohc REQUIRED)
find_package(zenohcxx REQUIRED)
find_package(ws_protocol REQUIRED)
find_package(OpenSSL REQUIRED)

# library target
file(GLOB_RECURSE SOURCES src/*.cpp include/hephaestus/ws_bridge/*.h)
define_module_library(
        NAME ws_bridge
        PUBLIC_LINK_LIBS
                hephaestus::action_server_types_proto
                hephaestus::cli
                hephaestus::concurrency
                hephaestus::containers
                hephaestus::serdes
                hephaestus::telemetry
                hephaestus::utils
                ws_protocol::foxglove_websocket
                absl::base
                absl::log
                fmt::fmt
                nlohmann_json::nlohmann_json
                OpenSSL::SSL
                OpenSSL::Crypto
        PRIVATE_LINK_LIBS ""
        SOURCES ${SOURCES} README.md
        PUBLIC_INCLUDE_PATHS
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE_INCLUDE_PATHS ""
        SYSTEM_PRIVATE_INCLUDE_PATHS "")

# Subprojects
add_subdirectory(tests)
add_subdirectory(apps)
