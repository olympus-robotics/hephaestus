# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

define_module_executable(
  NAME zenoh_topic_list
  SOURCES zenoh_topic_list.cpp PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS ""
)

define_module_executable(
  NAME zenoh_node_list
  SOURCES zenoh_node_list.cpp PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS ""
)

define_module_executable(
  NAME zenoh_string_service_client
  SOURCES zenoh_string_service_client.cpp PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS ""
)

define_module_executable(
  NAME zenoh_topic_echo
  SOURCES zenoh_topic_echo.cpp PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS ""
)

define_module_example(
  NAME zenoh_ipc_graph
  SOURCES zenoh_ipc_graph.cpp PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS ""
)
