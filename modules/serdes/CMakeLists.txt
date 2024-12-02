# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME serdes
  DEPENDS_ON_MODULES utils
  DEPENDS_ON_EXTERNAL_PROJECTS absl magic_enum Protobuf range-v3
)

find_package(absl REQUIRED)
find_package(magic_enum REQUIRED)
find_package(Protobuf REQUIRED)
find_package(utf8_range REQUIRED)
find_package(range-v3 REQUIRED)

# library sources
set(SOURCES
    src/protobuf/buffers.cpp
    src/protobuf/dynamic_deserializer.cpp
    src/protobuf/enums.cpp
    src/protobuf/protobuf_internal.cpp
    src/protobuf/protobuf.cpp
    src/dynamic_deserializer.cpp
    src/serdes.cpp
    src/type_info.cpp
    README.md
    include/hephaestus/serdes/protobuf/buffers.h
    include/hephaestus/serdes/protobuf/concepts.h
    include/hephaestus/serdes/protobuf/dynamic_deserializer.h
    include/hephaestus/serdes/protobuf/enums.h
    include/hephaestus/serdes/protobuf/protobuf_internal.h
    include/hephaestus/serdes/protobuf/protobuf.h
    include/hephaestus/serdes/dynamic_deserializer.h
    include/hephaestus/serdes/serdes.h
    include/hephaestus/serdes/type_info.h
)

# library target
define_module_library(
  NAME serdes
  PUBLIC_LINK_LIBS
    absl::log_internal_check_op
    absl::raw_hash_set
    absl::status
    absl::statusor
    hephaestus::utils
    magic_enum::magic_enum
    protobuf::libprotobuf
    range-v3::range-v3
    utf8_range::utf8_validity
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
