# =================================================================================================
# Copyright (C) 2025 HEPHAESTUS Contributors
# =================================================================================================

define_module_test(
  NAME generic_formatter_tests
  SOURCES generic_formatter_tests.cpp
  PUBLIC_LINK_LIBS hephaestus::types hephaestus::types_proto
)

define_module_test(
  NAME formatter_collisions_tests
  SOURCES formatter_collisions_tests.cpp
  PUBLIC_LINK_LIBS ""
)

define_module_test(
  NAME types_formatter_tests
  SOURCES types_formatter_tests.cpp
  PUBLIC_LINK_LIBS hephaestus::types
)
