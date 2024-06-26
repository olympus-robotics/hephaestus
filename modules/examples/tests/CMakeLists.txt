#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

define_module_test(
        NAME types_protobuf_tests
        SOURCES types_protobuf_tests.cpp helpers.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")


# TODO: This test should be in the IPC package, as soon as we have a proto helper package in there
define_module_test(
        NAME zenoh_tests
        SOURCES zenoh_tests.cpp helpers.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")
