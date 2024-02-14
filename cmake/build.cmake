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
include(${CMAKE_TEMPLATES_DIR}/06_packager_config.cmake)

# -------------------------------------------------------------------------------------------------
# Enumerate all modules and those selected for build (with -DBUILD_MODULES)
enumerate_modules(ROOT_PATH ${PROJECT_SOURCE_DIR}/modules)

# -------------------------------------------------------------------------------------------------
# Build external dependencies before configuring project

# define staging directories for external projects
set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/external) # location of build tree for external projects
set(EP_DEPLOY_DIR ${EP_BINARY_DIR}/deploy) # location of installable files from external projects
file(MAKE_DIRECTORY ${EP_DEPLOY_DIR})
get_property(_external_projects_list GLOBAL PROPERTY EXTERNAL_PROJECTS)

# Because execute_process() cannot handle ';' character in a list when passed as argument, escape it
string(REPLACE ";" "\\;" formatted_external_projects_list "${_external_projects_list}")

# Configuration of all external dependencies are in external/CMakeLists.txt NOTE: external/CMakeLists.txt is processed
# as if it was a separate project. This means: - Variables set there are not shared by the rest of this project - CMake
# parameters must be explicitly passed as if cmake was called on it from the command line
message(STATUS "========= External dependencies: Configuring =========")
set(EXTERNAL_PREFIX_PATH "${CMAKE_PREFIX_PATH} ${CMAKE_INSTALL_PREFIX}")
execute_process(
  COMMAND
    ${CMAKE_COMMAND} -G "Ninja" ${PROJECT_SOURCE_DIR}/external # Use 'Ninja' for parallel build
    -DEXTERNAL_PROJECTS_LIST=${formatted_external_projects_list} -DCMAKE_INSTALL_RPATH=${CMAKE_INSTALL_RPATH}
    -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_PREFIX_PATH=${EXTERNAL_PREFIX_PATH} -DCMAKE_INSTALL_PREFIX=${EP_DEPLOY_DIR}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
    -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER} -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
    -DCMAKE_TEMPLATE_DIR=${CMAKE_TEMPLATES_DIR}
  WORKING_DIRECTORY ${EP_BINARY_DIR}
  RESULT_VARIABLE _result
)

# if configuration succeeded, then build all external projects
if(${_result} EQUAL 0)
  message(STATUS "========= External dependencies: Building    =========")
  execute_process(
    COMMAND ${CMAKE_COMMAND} --build .
    WORKING_DIRECTORY ${EP_BINARY_DIR}
    RESULT_VARIABLE _result
  )
endif()

# If anything went wrong with external project build, stop and exit
if(NOT ${_result} EQUAL 0)
  message(FATAL_ERROR "Error processing ${PROJECT_SOURCE_DIR}/external/CMakeLists.txt")
endif()

# list(PREPEND CMAKE_PREFIX_PATH ${EP_DEPLOY_DIR}/)
install(
  DIRECTORY ${EP_DEPLOY_DIR}/
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  USE_SOURCE_PERMISSIONS
)
if(CMAKE_CROSSCOMPILING)
  list(APPEND CMAKE_FIND_ROOT_PATH ${EP_DEPLOY_DIR})
endif()

message(STATUS "======= External dependencies: Completed build =======")

# -------------------------------------------------------------------------------------------------
# Configure modules enabled with -DBUILD_MODULES="semi-colon;separated;list". - Special value "all" will build all
# modules found
if(NOT BUILD_MODULES)
  message(STATUS "BUILD_MODULES not specified. Only modules set to ALWAYS_BUILD will be enabled.")
else()
  message(STATUS "Requested modules (BUILD_MODULES): ${BUILD_MODULES}")
endif()
message(STATUS "----------------------------------------------------------------------")
message(STATUS "Configuring modules")
message(STATUS "----------------------------------------------------------------------")
configure_modules()
