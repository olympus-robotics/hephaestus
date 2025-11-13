# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME telemetry
  DEPENDS_ON_MODULES containers
  DEPENDS_ON_EXTERNAL_PROJECTS fmt magic_enum absl reflectcpp
)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)
find_package(magic_enum REQUIRED)
find_package(reflectcpp REQUIRED)

# library sources
set(SOURCES
    src/metric_sink.cpp
    src/metric_record.cpp
    src/log_sink.cpp
    src/log.cpp
    src/scope.cpp
    src/log_sinks/absl_sink.cpp
    src/metric_sinks/terminal_sink.cpp
    include/hephaestus/telemetry/metrics/metric_sink.h
    include/hephaestus/telemetry/metrics/metric_record.h
    include/hephaestus/telemetry/log/log_sink.h
    include/hephaestus/telemetry/log/log.h
    include/hephaestus/telemetry/log/scope.h
    include/hephaestus/telemetry/log/sinks/absl_sink.h
    include/hephaestus/telemetry/metrics/sinks/terminal_sink.h
)

# library target
define_module_library(
  NAME telemetry
  PUBLIC_LINK_LIBS
    absl::base
    absl::log
    absl::synchronization
    fmt::fmt
    hephaestus::containers
    hephaestus::utils
    magic_enum::magic_enum
    reflectcpp::reflectcpp
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)
