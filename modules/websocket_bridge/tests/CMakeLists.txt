#=================================================================================================
# Copyright (C) 2025 HEPHAESTUS Contributors
#=================================================================================================

define_module_test(
        NAME bridge_config_tests
        SOURCES bridge_config_tests.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS 
                hephaestus::websocket_bridge
                hephaestus::types
                hephaestus::types_proto
)

define_module_test(
        NAME bridge_state_tests
        SOURCES bridge_state_tests.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS 
                hephaestus::websocket_bridge
                hephaestus::types
                hephaestus::types_proto
)

define_module_test(
        NAME ipc_graph_tests
        SOURCES ipc_graph_tests.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS 
                hephaestus::websocket_bridge
                hephaestus::types
                hephaestus::types_proto
)

define_module_test(
        NAME ipc_entity_manager_tests
        SOURCES ipc_entity_manager_tests.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS 
                hephaestus::websocket_bridge
                hephaestus::types
                hephaestus::types_proto
)

define_module_test(
        NAME protobuf_serdes_tests
        SOURCES protobuf_serdes_tests.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS 
                hephaestus::websocket_bridge
                hephaestus::types
                hephaestus::types_proto
)
