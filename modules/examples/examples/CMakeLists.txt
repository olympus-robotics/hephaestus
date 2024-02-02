#=================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
#=================================================================================================

define_module_example(
        NAME zenoh_pub
        SOURCES zenoh_pub.cpp zenoh_program_options.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS eolo::ipc)

define_module_example(
        NAME zenoh_sub
        SOURCES zenoh_sub.cpp zenoh_program_options.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS eolo::ipc)