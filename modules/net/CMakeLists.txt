# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME net
  DEPENDS_ON_MODULES concurrency
  DEPENDS_ON_EXTERNAL_PROJECTS libbluetooth
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBBLUETOOTH REQUIRED IMPORTED_TARGET libbluetooth)

# library sources
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h include/*.h)

# library target
define_module_library(
  NAME net
  PUBLIC_LINK_LIBS hephaestus::concurrency PkgConfig::LIBBLUETOOTH
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects add_subdirectory(tests) add_subdirectory(examples)
