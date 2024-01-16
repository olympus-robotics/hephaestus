#=================================================================================================
# Copyright (C) 2018-2023 EOLO Contributors
#=================================================================================================

define_module_test(
        NAME tests
        SOURCES utils_tests.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")
