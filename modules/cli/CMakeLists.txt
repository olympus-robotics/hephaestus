# =================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
# =================================================================================================

declare_module(
  NAME cli
  DEPENDS_ON_MODULES base
  DEPENDS_ON_EXTERNAL_PROJECTS fmt
)

find_package(fmt REQUIRED)

# library sources
set(SOURCES
    src/keyboard.cpp
    src/program_options.cpp
    README.md
    include/eolo/cli/keyboard.h
    include/eolo/cli/program_options.h
)

# library target
define_module_library(
  NAME cli
  PUBLIC_LINK_LIBS eolo::base fmt::fmt
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
