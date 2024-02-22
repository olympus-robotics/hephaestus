# =================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
# =================================================================================================

declare_module(
  NAME bag
  DEPENDS_ON_MODULES containers ipc
  DEPENDS_ON_EXTERNAL_PROJECTS mcap
)

find_package(mcap REQUIRED)

# library sources
set(SOURCES src/zenoh_recorder.cpp src/writer.cpp README.md include/eolo/bag/zenoh_recorder.h include/eolo/bag/writer.h)

# library target
define_module_library(
  NAME bag
  PUBLIC_LINK_LIBS eolo::containers  eolo::ipc mcap
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
