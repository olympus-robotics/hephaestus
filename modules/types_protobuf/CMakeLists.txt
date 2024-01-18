#=================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
#=================================================================================================

declare_module(
    NAME types_protobuf
    DEPENDS_ON_MODULES base serdes types
    DEPENDS_ON_EXTERNAL_PROJECTS Protobuf Eigen3
)

find_package(Eigen3 REQUIRED)

add_subdirectory(proto)

# library sources
set(SOURCES
        src/geometry.cpp
        src/pose.cpp
        README.md
        include/eolo/types_protobuf/geometry.h
        include/eolo/types_protobuf/pose.h)

# library target
define_module_library(
        NAME types_protobuf
        PUBLIC_LINK_LIBS
            eolo::base
            eolo::serdes
            eolo::types
            eolo::types_proto
            Eigen3::Eigen
        PRIVATE_LINK_LIBS ""
        SOURCES ${SOURCES}
        PUBLIC_INCLUDE_PATHS
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE_INCLUDE_PATHS ""
        SYSTEM_PRIVATE_INCLUDE_PATHS "")

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
