# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME telemetry_loki_sink
  DEPENDS_ON_MODULES telemetry
  DEPENDS_ON_EXTERNAL_PROJECTS cpr fmt reflectcpp
)

find_package(cpr REQUIRED)
find_package(fmt REQUIRED)
find_package(reflectcpp REQUIRED)

# library sources
set(SOURCES src/loki_log_sink.cpp include/hephaestus/telemetry/telemetry_loki_sink/loki_log_sink.h)

# library target
define_module_library(
  NAME telemetry_loki_sink
  PUBLIC_LINK_LIBS fmt::fmt hephaestus::telemetry cpr::cpr reflectcpp::reflectcpp
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

add_subdirectory(examples)
