#=================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
#=================================================================================================

declare_module(NAME types DEPENDS_ON_MODULES "" DEPENDS_ON_EXTERNAL_PROJECTS Eigen3)

find_package(Eigen3 REQUIRED)

# library sources
set(SOURCES
        src/pose.cpp
        README.md
        include/eolo/types/pose.h)

# library target
define_module_library(
        NAME types
        PUBLIC_LINK_LIBS
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
