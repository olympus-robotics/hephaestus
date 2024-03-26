# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME serdes
  DEPENDS_ON_MODULES base
  DEPENDS_ON_EXTERNAL_PROJECTS absl magic_enum Protobuf
)

find_package(absl REQUIRED)
find_package(magic_enum REQUIRED)
find_package(Protobuf REQUIRED)

# library sources
set(SOURCES
    src/serdes.cpp
    src/type_info.cpp
    src/dynamic_deserializer.cpp
    src/protobuf/dynamic_deserializer.cpp
    src/protobuf/buffers.cpp
    src/protobuf/protobuf.cpp
    src/protobuf/protobuf_internal.cpp
    README.md
    include/hephaestus/serdes/dynamic_deserializer.h
    include/hephaestus/serdes/serdes.h
    include/hephaestus/serdes/type_info.h
    include/hephaestus/serdes/protobuf/concepts.h
    include/hephaestus/serdes/protobuf/buffers.h
    include/hephaestus/serdes/protobuf/dynamic_deserializer.h
    include/hephaestus/serdes/protobuf/protobuf.h
    include/hephaestus/serdes/protobuf/protobuf_internal.h
)

# library target
define_module_library(
  NAME serdes
  PUBLIC_LINK_LIBS absl::status hephaestus::base magic_enum::magic_enum protobuf::libprotobuf
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
