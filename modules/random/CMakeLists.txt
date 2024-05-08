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
set(SOURCES
    src/random_container.cpp
    src/random_generator.cpp
    src/random_type.cpp
    README.md
    include/hephaestus/random/random_container.h
    include/hephaestus/random/random_generator.h
    include/hephaestus/random/random_type.h
)

# library target
define_module_library(
  NAME random
  PUBLIC_LINK_LIBS fmt::fmt magic_enum::magic_enum hephaestus::utils
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
