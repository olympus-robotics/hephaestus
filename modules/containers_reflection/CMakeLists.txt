# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME containers_reflection
  DEPENDS_ON_MODULES containers utils telemetry
  DEPENDS_ON_EXTERNAL_PROJECTS reflectcpp fmt
)

find_package(fmt REQUIRED)
find_package(reflectcpp REQUIRED)

# library sources
set(SOURCES src/bit_flag.cpp src/chrono.cpp include/hephaestus/containers_reflection/bit_flag.h
            include/hephaestus/containers_reflection/chrono.h
)

# library target
define_module_library(
  NAME containers_reflection
  PUBLIC_LINK_LIBS fmt::fmt hephaestus::utils reflectcpp::reflectcpp
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
