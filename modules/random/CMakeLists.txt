# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME random
  DEPENDS_ON_MODULES utils
  DEPENDS_ON_EXTERNAL_PROJECTS ""
)

find_package(fmt REQUIRED)
find_package(magic_enum REQUIRED)

# library sources
set(SOURCES src/random_number_generator.cpp src/random_object_creator.cpp README.md
            include/hephaestus/random/random_number_generator.h include/hephaestus/random/random_object_creator.h
)

# library target
define_module_library(
  NAME random
  PUBLIC_LINK_LIBS
    fmt::fmt
    hephaestus::utils
    magic_enum::magic_enum
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
