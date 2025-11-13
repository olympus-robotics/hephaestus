# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME telemetry_log
  DEPENDS_ON_MODULES containers
  DEPENDS_ON_EXTERNAL_PROJECTS fmt magic_enum absl
)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)
find_package(magic_enum REQUIRED)

# library sources
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h include/*.h)

# library target
define_module_library(
  NAME telemetry_log
  PUBLIC_LINK_LIBS
    absl::base
    absl::log
    absl::synchronization
    fmt::fmt
    hephaestus::containers
    hephaestus::utils
    magic_enum::magic_enum
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)
