#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

define_module_proto_library(NAME telemetry_examples_proto SOURCES example.proto NOINSTALL)

define_module_example(
        NAME example
        SOURCES example.cpp example_proto_conversion.h
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS hephaestus::telemetry_examples_proto hephaestus::random)
