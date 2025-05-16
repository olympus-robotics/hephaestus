# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME concurrency
  DEPENDS_ON_MODULES utils containers random
  DEPENDS_ON_EXTERNAL_PROJECTS absl fmt Protobuf
)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)
find_package(Protobuf REQUIRED CONFIG)

# library sources
set(SOURCES
    src/message_queue_consumer.cpp
    src/spinner.cpp
    src/spinners_manager.cpp
    README.md
    include/hephaestus/concurrency/message_queue_consumer.h
    include/hephaestus/concurrency/spinner.h
    include/hephaestus/concurrency/spinners_manager.h
)

# library target
define_module_library(
  NAME concurrency
  PUBLIC_LINK_LIBS
    fmt::fmt
    hephaestus::containers
    hephaestus::telemetry
    hephaestus::utils
    protobuf::libprotobuf
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
