# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME test_helpers
  DEPENDS_ON_MODULES random
  DEPENDS_ON_EXTERNAL_PROJECTS fmt magic_enum
)

find_package(fmt REQUIRED)
find_package(magic_enum REQUIRED)

# library sources
set(SOURCES  src/dummy_type.cpp README.md
            include/hephaestus/test_helpers/dummy_type.h
)

# library target
define_module_library(
  NAME test_helpers
  PUBLIC_LINK_LIBS fmt::fmt magic_enum::magic_enum hephaestus::random
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
