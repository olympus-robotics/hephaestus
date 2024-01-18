#=================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
#=================================================================================================

declare_module(NAME ipc DEPENDS_ON_MODULES base types_protobuf DEPENDS_ON_EXTERNAL_PROJECTS zenohcxx)

find_package(zenohcxx REQUIRED)

# library sources
set(SOURCES
        src/zenoh.cpp
        README.md
        include/eolo/ipc/zenoh.h)

# library target
define_module_library(
        NAME ipc
        PUBLIC_LINK_LIBS
            eolo::base
            eolo::types_protobuf
            # zenohcxx
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
