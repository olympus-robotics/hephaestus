# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME types_protobuf
  DEPENDS_ON_MODULES random serdes types 
  DEPENDS_ON_EXTERNAL_PROJECTS ""
)

add_subdirectory(proto)

# library sources
set(SOURCES src/dummy_type.cpp README.md include/hephaestus/types_protobuf/dummy_type.h
)

# library target
define_module_library(
  NAME types_protobuf
  PUBLIC_LINK_LIBS hephaestus::random hephaestus::serdes hephaestus::types hephaestus::types_proto
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
