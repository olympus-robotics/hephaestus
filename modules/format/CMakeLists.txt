# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME format
  DEPENDS_ON_MODULES utils
  DEPENDS_ON_EXTERNAL_PROJECTS ""
)

find_package(fmt REQUIRED)

# library sources
set(SOURCES src/generic_formatter.cpp README.md include/hephaestus/format/generic_formatter.h)

# library target
define_module_library(
  NAME format
  PUBLIC_LINK_LIBS fmt::fmt
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
