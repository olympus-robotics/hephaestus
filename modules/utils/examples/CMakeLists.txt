#=================================================================================================
# Copyright (C) 2018-2023 EOLO Contributors
#=================================================================================================


define_module_example(
        NAME utils_example
        SOURCES utils_example.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")
