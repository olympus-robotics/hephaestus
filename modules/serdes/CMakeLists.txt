# =================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
# =================================================================================================

declare_module(
  NAME serdes
  DEPENDS_ON_MODULES base
  DEPENDS_ON_EXTERNAL_PROJECTS magic_enum Protobuf
)

find_package(magic_enum REQUIRED)
find_package(Protobuf REQUIRED)

# library sources
set(SOURCES
    src/serdes.cpp
    src/type_info.cpp
    src/protobuf/buffers.cpp
    src/protobuf/protobuf.cpp
    README.md
    include/eolo/serdes/serdes.h
    include/eolo/serdes/type_info.h
    include/eolo/serdes/protobuf/concepts.h
    include/eolo/serdes/protobuf/buffers.h
    include/eolo/serdes/protobuf/protobuf.h
)

# library target
define_module_library(
  NAME serdes
  PUBLIC_LINK_LIBS eolo::base magic_enum::magic_enum protobuf::libprotobuf
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
