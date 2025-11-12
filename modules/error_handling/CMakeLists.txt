# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME error_handling
  DEPENDS_ON_MODULES telemetry utils
  DEPENDS_ON_EXTERNAL_PROJECTS fmt
)

find_package(fmt REQUIRED)

# library sources
set(SOURCES src/panic.cpp include/hephaestus/error_handling/panic.h)

# library target
define_module_library(
  NAME error_handling
  PUBLIC_LINK_LIBS fmt::fmt hephaestus::telemetry hephaestus::utils reflectcpp::reflectcpp
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

add_subdirectory(examples)
add_subdirectory(tests)
