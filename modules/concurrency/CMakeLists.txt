# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME concurrency
  DEPENDS_ON_MODULES base
  DEPENDS_ON_EXTERNAL_PROJECTS absl fmt
)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)

# library sources
set(SOURCES 
  src/spinner.cpp 
  README.md 
  include/hephaestus/concurrency/spinner.h
)

# library target
define_module_library(
  NAME concurrency
  PUBLIC_LINK_LIBS 
    absl::log
    hephaestus::base 
    fmt::fmt
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
