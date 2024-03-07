#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

define_module_example(
        NAME zenoh_pub
        SOURCES zenoh_pub.cpp zenoh_program_options.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS hephaestus::ipc)

define_module_example(
        NAME zenoh_sub
        SOURCES zenoh_sub.cpp zenoh_program_options.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS hephaestus::ipc)

define_module_example(
        NAME zenoh_sub_any
        SOURCES zenoh_sub_any.cpp zenoh_program_options.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS hephaestus::ipc)

define_module_example(
        NAME mcap_writer
        SOURCES mcap_writer.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS mcap)

define_module_example(
        NAME mcap_reader
        SOURCES mcap_reader.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS mcap)
