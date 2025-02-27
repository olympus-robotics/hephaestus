#=================================================================================================
# Copyright (C) 2025 HEPHAESTUS Contributors
#=================================================================================================

define_module_test(
        NAME test_bridge_config
        SOURCES test_bridge_config.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

define_module_test(
        NAME test_bridge_state
        SOURCES test_bridge_state.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

define_module_test(
        NAME test_ipc_graph
        SOURCES test_ipc_graph.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

