# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME conduit
  DEPENDS_ON_MODULES concurrency ipc
  DEPENDS_ON_EXTERNAL_PROJECTS ""
)

# library sources
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h include/*.h)

# library target
define_module_library(
  NAME conduit
  PUBLIC_LINK_LIBS hephaestus::concurrency hephaestus::ipc
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects add_subdirectory(tests) add_subdirectory(examples)
