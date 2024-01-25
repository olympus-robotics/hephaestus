#=================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
#=================================================================================================

define_module_executable(
        NAME zenoh_router
        SOURCES zenoh_router.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")
