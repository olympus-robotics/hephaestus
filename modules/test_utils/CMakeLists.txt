# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

if(BUILD_TESTING)
  declare_module(
    NAME test_utils
    DEPENDS_ON_MODULES error_handling utils random telemetry_metrics
    DEPENDS_ON_EXTERNAL_PROJECTS gtest
  )

# library sources
  set(SOURCES
    src/heph_test.cpp
    src/test_main.cpp
    include/hephaestus/test_utils/heph_test.h
  )

# library target
  define_module_library(
    NAME test_utils
    PUBLIC_LINK_LIBS hephaestus::error_handling hephaestus::utils GTest::gtest
    PRIVATE_LINK_LIBS hephaestus::random hephaestus::telemetry_metrics
    SOURCES ${SOURCES}
    PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
    PRIVATE_INCLUDE_PATHS ""
    SYSTEM_PRIVATE_INCLUDE_PATHS ""
  )
endif()
