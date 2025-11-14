# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME telemetry_influxdb_sink
  DEPENDS_ON_MODULES concurrency random serdes telemetry_metrics
  DEPENDS_ON_EXTERNAL_PROJECTS InfluxDB fmt
)

find_package(InfluxDB REQUIRED)
find_package(fmt REQUIRED)

# library sources
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h include/*.h)

# library target
define_module_library(
  NAME telemetry_influxdb_sink
  PUBLIC_LINK_LIBS fmt::fmt hephaestus::concurrency hephaestus::random hephaestus::serdes hephaestus::telemetry_metrics
                   InfluxData::InfluxDB
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

add_subdirectory(examples)
