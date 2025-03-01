#=================================================================================================
# Copyright (C) 2025 HEPHAESTUS Contributors
#=================================================================================================

declare_module(
        NAME websocket_bridge
        DEPENDS_ON_MODULES ipc cli concurrency containers utils serdes telemetry
        DEPENDS_ON_EXTERNAL_PROJECTS  absl fmt yaml-cpp zenohc zenohcxx ws_protocol reflectcpp)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(zenohc REQUIRED)
find_package(zenohcxx REQUIRED)
find_package(foxglove_websocket REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(reflectcpp REQUIRED)
find_package(ZLIB REQUIRED)

# library target
file(GLOB_RECURSE SOURCES 
        src/websocket_bridge/*.cpp
        src/ipc/*.cpp
        src/utils/*.cpp
        include/hephaestus/websocket_bridge/*.h
        include/hephaestus/ipc/*.h
        include/hephaestus/utils/*.h
)

define_module_library(
        NAME websocket_bridge
        PUBLIC_LINK_LIBS
                absl::base
                absl::log
                fmt::fmt
                foxglove::foxglove_websocket
                hephaestus::action_server_types_proto
                hephaestus::cli
                hephaestus::concurrency
                hephaestus::containers
                hephaestus::ipc
                hephaestus::serdes
                hephaestus::telemetry
                hephaestus::utils
                OpenSSL::Crypto
                OpenSSL::SSL
                reflectcpp::reflectcpp
                yaml-cpp
                ZLIB::ZLIB
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
add_subdirectory(examples)
