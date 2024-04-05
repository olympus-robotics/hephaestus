# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME ipc
  DEPENDS_ON_MODULES utils cli serdes
  DEPENDS_ON_EXTERNAL_PROJECTS absl fmt nlohmann_json range-v3 zenohcxx
)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)
find_package(range-v3 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(zenohc REQUIRED)
find_package(zenohcxx REQUIRED)

# library sources
set(SOURCES
    src/common.cpp
    src/topic_database.cpp
    src/zenoh/liveliness.cpp
    src/zenoh/publisher.cpp
    src/zenoh/query.cpp
    src/zenoh/service.cpp
    src/zenoh/scout.cpp
    src/zenoh/subscriber.cpp
    src/zenoh/utils.cpp
    README.md
    include/hephaestus/ipc/common.h
    include/hephaestus/ipc/topic_database.h
    include/hephaestus/ipc/zenoh/utils.h
    include/hephaestus/ipc/zenoh/publisher.h
    include/hephaestus/ipc/zenoh/query.h
    include/hephaestus/ipc/zenoh/service.h
    include/hephaestus/ipc/zenoh/scout.h
    include/hephaestus/ipc/zenoh/subscriber.h
)

# library target
define_module_library(
  NAME ipc
  PUBLIC_LINK_LIBS
    absl::base
    absl::status
    hephaestus::utils
    hephaestus::cli
    fmt::fmt
    nlohmann_json::nlohmann_json
    range-v3::range-v3
    hephaestus::serdes
    zenohc::lib
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(apps)
