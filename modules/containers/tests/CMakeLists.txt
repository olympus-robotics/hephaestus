#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

define_module_test(
  NAME bit_flag_tests
  SOURCES bit_flag_tests.cpp
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS ""
)

define_module_test(
  NAME blocking_queue_tests
  SOURCES blocking_queue_tests.cpp
  PUBLIC_INCLUDE_PATHS
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS ""
)
