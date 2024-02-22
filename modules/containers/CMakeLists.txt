# =================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
# =================================================================================================

declare_module(
  NAME containers
  DEPENDS_ON_MODULES base
  DEPENDS_ON_EXTERNAL_PROJECTS ""
)

# library sources
set(SOURCES src/blocking_queue.cpp README.md include/eolo/containers/blocking_queue.h)

# library target
define_module_library(
  NAME containers
  PUBLIC_LINK_LIBS eolo::base
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
