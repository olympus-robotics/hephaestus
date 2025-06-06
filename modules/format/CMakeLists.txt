# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME format
  DEPENDS_ON_MODULES utils containers_reflection
  DEPENDS_ON_EXTERNAL_PROJECTS reflectcpp fmt magic_enum
)

find_package(fmt REQUIRED)
find_package(reflectcpp REQUIRED)
find_package(magic_enum REQUIRED)

# library sources
set(SOURCES src/generic_formatter.cpp README.md include/hephaestus/format/generic_formatter.h
            include/hephaestus/format/enum.h
)

# library target
define_module_library(
  NAME format
  PUBLIC_LINK_LIBS fmt::fmt hephaestus::utils reflectcpp::reflectcpp magic_enum::magic_enum
                   hephaestus::containers_reflection
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
