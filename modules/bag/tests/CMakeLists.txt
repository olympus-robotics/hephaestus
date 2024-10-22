#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

define_module_proto_library(NAME bag_tests_proto SOURCES test_type.proto)

define_module_test(
        NAME tests
        SOURCES tests.cpp test_proto_conversion.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS hephaestus::bag_tests_proto hephaestus::random)
