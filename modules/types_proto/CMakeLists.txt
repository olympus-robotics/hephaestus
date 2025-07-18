# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME types_proto
  DEPENDS_ON_MODULES random serdes types
  DEPENDS_ON_EXTERNAL_PROJECTS ""
)

add_subdirectory(proto)

# library sources
set(SOURCES
    src/bool.cpp
    src/bounds.cpp
    src/dummy_type.cpp
    src/numeric_value.cpp
    src/uuid_v4.cpp
    README.md
    include/hephaestus/types_proto/bool.h
    include/hephaestus/types_proto/bounds.h
    include/hephaestus/types_proto/dummy_type.h
    include/hephaestus/types_proto/numeric_value.h
    include/hephaestus/types_proto/uuid_v4.h
)

# library target
define_module_library(
  NAME types_proto
  PUBLIC_LINK_LIBS hephaestus::random hephaestus::serdes hephaestus::types hephaestus::types_gen_proto
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
