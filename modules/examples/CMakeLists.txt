declare_module(
  NAME examples
  DEPENDS_ON_MODULES bag utils ipc random serdes
  DEPENDS_ON_EXTERNAL_PROJECTS absl Eigen3 fmt mcap Protobuf
  EXCLUDE_FROM_ALL
)

find_package(absl REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(fmt REQUIRED)
find_package(mcap REQUIRED)

add_subdirectory(proto)

# library sources
set(TYPES_SOURCES src/types/pose.cpp README.md include/hephaestus/examples/types/pose.h include/hephaestus/examples/types/sample.h )

# library target
define_module_library(
  NAME examples_types
  PUBLIC_LINK_LIBS absl::base absl::status Eigen3::Eigen hephaestus::bag fmt::fmt mcap
  PRIVATE_LINK_LIBS ""
  SOURCES ${TYPES_SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# library sources
set(TYPES_PROTOBUF_SOURCES
    src/types_protobuf/geometry.cpp src/types_protobuf/pose.cpp src/types_protobuf/sample.cpp include/hephaestus/examples/types_protobuf/geometry.h
    include/hephaestus/examples/types_protobuf/pose.h include/hephaestus/examples/types_protobuf/sample.h
)

# library target
define_module_library(
  NAME examples_types_protobuf
  PUBLIC_LINK_LIBS hephaestus::utils hephaestus::serdes hephaestus::random hephaestus::examples_types
                   hephaestus::examples_types_proto Eigen3::Eigen
  PRIVATE_LINK_LIBS ""
  SOURCES ${TYPES_PROTOBUF_SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
