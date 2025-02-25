#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

define_module_executable(
        NAME app
        SOURCES bridge_app.cpp
        PUBLIC_INCLUDE_PATHS
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

define_module_executable(
        NAME test_client__services
        SOURCES test_client__services.cpp
        PUBLIC_INCLUDE_PATHS
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")
