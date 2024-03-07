#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

define_module_executable(
        NAME zenoh_topic_list
        SOURCES zenoh_topic_list.cpp zenoh_program_options.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

define_module_executable(
        NAME zenoh_node_list
        SOURCES zenoh_node_list.cpp zenoh_program_options.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

define_module_executable(
        NAME zenoh_query
        SOURCES zenoh_query.cpp zenoh_program_options.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")
