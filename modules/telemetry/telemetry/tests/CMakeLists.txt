# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

define_module_test(
  NAME metric_record_tests
  SOURCES metric_record_tests.cpp
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS ""
)

define_module_test(
  NAME log_tests
  SOURCES log_tests.cpp
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS ""
)

define_module_test(
  NAME log_tests_no_sink
  SOURCES log_tests_no_sink.cpp
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  PUBLIC_LINK_LIBS ""
)
