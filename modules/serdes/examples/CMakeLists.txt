#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

define_module_example(
        NAME example
        SOURCES example.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")
