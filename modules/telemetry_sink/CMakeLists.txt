# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME telemetry_sink
  DEPENDS_ON_MODULES concurrency random serdes telemetry
  DEPENDS_ON_EXTERNAL_PROJECTS influxdb fmt
  )

find_package(InfluxDB REQUIRED)
find_package(fmt REQUIRED)

# library sources
set(SOURCES
    src/influxdb_measure_sink.cpp
    include/hephaestus/telemetry_sink/influxdb_measure_sink.h
)

# library target
define_module_library(
  NAME telemetry_sink
  PUBLIC_LINK_LIBS hephaestus::concurrency hephaestus::random hephaestus::serdes hephaestus::telemetry InfluxData::InfluxDB fmt::fmt
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# add_subdirectory(tests)
