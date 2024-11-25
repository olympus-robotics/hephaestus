# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME cli
  DEPENDS_ON_MODULES utils
  DEPENDS_ON_EXTERNAL_PROJECTS absl fmt
)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)

# library sources
set(SOURCES src/keyboard.cpp src/program_options.cpp README.md include/hephaestus/cli/keyboard.h
            include/hephaestus/cli/program_options.h
)

# library target
define_module_library(
  NAME cli
  PUBLIC_LINK_LIBS hephaestus::utils fmt::fmt absl::strings
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
