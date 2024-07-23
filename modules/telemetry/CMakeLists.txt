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
set(SOURCES
    src/measure_sink.cpp
    src/measure.cpp
    include/hephaestus/telemetry/measure_sink.h
    include/hephaestus/telemetry/measure.h
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
