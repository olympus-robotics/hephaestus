#=================================================================================================
# Copyright (C) 2025 HEPHAESTUS Contributors
#=================================================================================================


define_module_executable(
        NAME example_client__services
        SOURCES example_client__services.cpp
        PUBLIC_INCLUDE_PATHS
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

define_module_executable(
        NAME example_client__topics
        SOURCES example_client__topics.cpp
        PUBLIC_INCLUDE_PATHS
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

