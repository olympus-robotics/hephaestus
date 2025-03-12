# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

define_module_example(
  NAME zenoh_pub
  SOURCES zenoh_pub.cpp zenoh_program_options.h
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS hephaestus::ipc
)

define_module_example(
  NAME zenoh_service_client
  SOURCES zenoh_service_client.cpp zenoh_program_options.h
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS hephaestus::ipc
)

define_module_example(
  NAME zenoh_service_server
  SOURCES zenoh_service_server.cpp zenoh_program_options.h
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS hephaestus::ipc
)

define_module_example(
  NAME zenoh_string_service_client
  SOURCES zenoh_string_service_client.cpp zenoh_program_options.h
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS hephaestus::ipc
)

define_module_example(
  NAME zenoh_string_service_server
  SOURCES zenoh_string_service_server.cpp zenoh_program_options.h
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS hephaestus::ipc
)

define_module_example(
  NAME zenoh_action_server
  SOURCES zenoh_action_server.cpp zenoh_program_options.h
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS hephaestus::ipc
)

define_module_example(
  NAME zenoh_action_server_client
  SOURCES zenoh_action_server_client.cpp zenoh_program_options.h
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS hephaestus::ipc
)

define_module_example(
  NAME zenoh_sub
  SOURCES zenoh_sub.cpp zenoh_program_options.h
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS hephaestus::ipc
)

define_module_example(
  NAME mcap_writer
  SOURCES mcap_writer.cpp
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS mcap
)

define_module_example(
  NAME mcap_reader
  SOURCES mcap_reader.cpp
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS mcap
)
