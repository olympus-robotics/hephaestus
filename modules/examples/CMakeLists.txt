declare_module(
  NAME examples
  DEPENDS_ON_MODULES
    bag
    utils
    ipc
    random
    serdes
    types_proto
    types
    error_handling
  DEPENDS_ON_EXTERNAL_PROJECTS absl Eigen3 fmt mcap Protobuf zstd
  EXCLUDE_FROM_ALL
)

find_package(absl REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(fmt REQUIRED)
find_package(mcap REQUIRED)
find_package(Protobuf REQUIRED CONFIG)
find_package(zstd REQUIRED)

add_subdirectory(proto)

# library sources
set(TYPES_SOURCES src/types/pose.cpp README.md include/hephaestus/examples/types/pose.h
                  include/hephaestus/examples/types/sample.h
)

# library target
define_module_library(
  NAME examples_types
  PUBLIC_LINK_LIBS
    absl::base
    absl::status
    fmt::fmt
    Eigen3::Eigen
    hephaestus::bag
    hephaestus::error_handling
    hephaestus::action_server_types_proto
    mcap
    zstd::libzstd
  PRIVATE_LINK_LIBS ""
  SOURCES ${TYPES_SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# library sources
set(types_proto_SOURCES
    src/types_proto/geometry.cpp src/types_proto/pose.cpp src/types_proto/sample.cpp
    include/hephaestus/examples/types_proto/geometry.h include/hephaestus/examples/types_proto/pose.h
    include/hephaestus/examples/types_proto/sample.h
)

# library target
define_module_library(
  NAME examples_types_proto
  PUBLIC_LINK_LIBS
    Eigen3::Eigen
    hephaestus::utils
    hephaestus::serdes
    hephaestus::random
    hephaestus::examples_types
    hephaestus::examples_types_gen_proto
    protobuf::libprotobuf
  PRIVATE_LINK_LIBS ""
  SOURCES ${types_proto_SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
