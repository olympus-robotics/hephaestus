# =================================================================================================
# Copyright (C) 2018 GRAPE Contributors Copyright (C) 2023-2024 EOLO Contributors
# =================================================================================================

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Required for shared libraries

# Top level CMake file for the Eolo Build System. See README.md

set(CMAKE_TEMPLATES_DIR ${CMAKE_CURRENT_LIST_DIR})

include(${CMAKE_TEMPLATES_DIR}/01_version.cmake)
include(${CMAKE_TEMPLATES_DIR}/02_build_config.cmake)
include(${CMAKE_TEMPLATES_DIR}/03_compiler_config.cmake)
include(${CMAKE_TEMPLATES_DIR}/04_code_formatter.cmake)
include(${CMAKE_TEMPLATES_DIR}/05_modules.cmake)
include(${CMAKE_TEMPLATES_DIR}/06_external_dependencies.cmake)
include(${CMAKE_TEMPLATES_DIR}/07_packager_config.cmake)

# -------------------------------------------------------------------------------------------------
# Enumerate all modules and those selected for build (with -DBUILD_MODULES)
enumerate_modules(ROOT_PATH ${PROJECT_SOURCE_DIR}/modules)

# -------------------------------------------------------------------------------------------------
# Build external dependencies before configuring project
build_external_dependencies(FOLDER ${PROJECT_SOURCE_DIR}/external)

# -------------------------------------------------------------------------------------------------
# Configure modules enabled with -DBUILD_MODULES="semi-colon;separated;list". - Special value "all" will build all
# modules found
configure_modules()
