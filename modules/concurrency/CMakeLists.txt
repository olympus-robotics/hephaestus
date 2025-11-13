# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME concurrency
  DEPENDS_ON_MODULES utils containers random error_handling telemetry_metrics
  DEPENDS_ON_EXTERNAL_PROJECTS absl fmt Protobuf liburing stdexec
)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)
find_package(Protobuf REQUIRED CONFIG)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBURING REQUIRED IMPORTED_TARGET liburing)
find_package(stdexec REQUIRED)

# library sources
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h include/*.h)

# library target
define_module_library(
  NAME concurrency
  PUBLIC_LINK_LIBS
    fmt::fmt
    hephaestus::containers
    hephaestus::error_handling
    hephaestus::telemetry_metrics
    hephaestus::utils
    protobuf::libprotobuf
    PkgConfig::LIBURING
    STDEXEC::stdexec
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
