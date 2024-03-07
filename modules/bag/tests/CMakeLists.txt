#=================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
#=================================================================================================

define_module_test(
        NAME topic_filter_tests
        SOURCES topic_filter_tests.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")
