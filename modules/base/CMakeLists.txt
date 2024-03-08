# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME base
  DEPENDS_ON_MODULES utils
  DEPENDS_ON_EXTERNAL_PROJECTS ""
)

# library sources
set(SOURCES src/exception.cpp README.md include/hephaestus/base/exception.h)

# library target
define_module_library(
  NAME base
  PUBLIC_LINK_LIBS hephaestus::utils
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
