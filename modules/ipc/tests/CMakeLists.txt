#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

define_module_test(
        NAME dynamic_subscriber_tests
        SOURCES dynamic_subscriber_tests.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

define_module_test(
        NAME topic_filter_tests
        SOURCES topic_filter_tests.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

define_module_test(
          NAME action_server_tests
          SOURCES action_server_tests.cpp
          PUBLIC_INCLUDE_PATHS
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
          PUBLIC_LINK_LIBS "")
