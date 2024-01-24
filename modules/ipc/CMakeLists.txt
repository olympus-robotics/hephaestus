#=================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
#=================================================================================================

declare_module(NAME ipc DEPENDS_ON_MODULES base cli types_protobuf DEPENDS_ON_EXTERNAL_PROJECTS zenohcxx)

find_package(zenohc REQUIRED)
find_package(zenohcxx REQUIRED)

# library sources
set(SOURCES
        src/common.cpp
        src/utils.cpp
        src/publisher.cpp
        src/subscriber.cpp
        README.md
        include/eolo/ipc/common.h
        include/eolo/ipc/zenoh/utils.h
        include/eolo/ipc/zenoh/publisher.h
        include/eolo/ipc/zenoh/subscriber.h)

# library target
define_module_library(
        NAME ipc
        PUBLIC_LINK_LIBS
            eolo::base
            eolo::cli
            eolo::types_protobuf
            zenohc::lib
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
