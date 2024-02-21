declare_module(
  NAME examples
  DEPENDS_ON_MODULES base ipc serdes
  DEPENDS_ON_EXTERNAL_PROJECTS Eigen3 fmt mcap Protobuf
  EXCLUDE_FROM_ALL
)

find_package(Eigen3 REQUIRED)
find_package(mcap REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(proto)

# library sources
set(TYPES_SOURCES src/types/pose.cpp README.md include/eolo/examples/types/pose.h)

# library target
define_module_library(
  NAME examples_types
  PUBLIC_LINK_LIBS Eigen3::Eigen fmt::fmt
  PRIVATE_LINK_LIBS ""
  SOURCES ${TYPES_SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# library sources
set(TYPES_PROTOBUF_SOURCES src/types_protobuf/geometry.cpp src/types_protobuf/pose.cpp
                           include/eolo/examples/types_protobuf/geometry.h include/eolo/examples/types_protobuf/pose.h
)

# library target
define_module_library(
  NAME examples_types_protobuf
  PUBLIC_LINK_LIBS eolo::base eolo::serdes eolo::examples_types eolo::examples_types_proto Eigen3::Eigen
  PRIVATE_LINK_LIBS ""
  SOURCES ${TYPES_PROTOBUF_SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
