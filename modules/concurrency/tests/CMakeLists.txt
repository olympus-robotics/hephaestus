#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

define_module_test(
        NAME queue_consumer_tests
        SOURCES queue_consumer_tests.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS hephaestus::random)

define_module_test(
        NAME spinner_tests
        SOURCES spinner_tests.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

define_module_test(
          NAME sync_tests
          SOURCES sync_tests.cpp
          PUBLIC_INCLUDE_PATHS
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
          PUBLIC_LINK_LIBS "")
