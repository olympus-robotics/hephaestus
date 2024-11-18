# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME telemetry
  DEPENDS_ON_MODULES concurrency random serdes
  DEPENDS_ON_EXTERNAL_PROJECTS fmt
)

find_package(fmt REQUIRED)

# library sources
set(SOURCES src/metric_sink.cpp src/metric_record.cpp src/metric_sinks/terminal_sink.cpp include/hephaestus/telemetry/metric_sink.h
            include/hephaestus/telemetry/metric_record.h include/hephaestus/telemetry/metric_sinks/terminal_sink.h
)

# library target
define_module_library(
  NAME telemetry
  PUBLIC_LINK_LIBS hephaestus::concurrency hephaestus::random hephaestus::serdes fmt::fmt
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

add_subdirectory(tests)
add_subdirectory(examples)
