# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME telemetry
  DEPENDS_ON_MODULES random serdes
  DEPENDS_ON_EXTERNAL_PROJECTS cpr influxdb fmt
)

add_subdirectory(proto)

find_package(cpr REQUIRED)
find_package(InfluxDB REQUIRED)
find_package(fmt REQUIRED)

# library sources
set(SOURCES
    src/influxdb_sink.cpp
    src/telemetry.cpp
    src/rest_sink.cpp
    src/terminal_sink.cpp
    src/proto_conversion.h
    src/proto_conversion.cpp
    README.md
    include/hephaestus/telemetry/sink.h
    include/hephaestus/telemetry/telemetry.h
)

# library target
define_module_library(
  NAME telemetry
  PUBLIC_LINK_LIBS hephaestus::serdes hephaestus::telemetry_proto cpr::cpr InfluxData::InfluxDB fmt::fmt
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(apps)
