# =================================================================================================
# Copyright (C) 2018-2023 EOLO Contributors
# =================================================================================================

# =================================================================================================
# This is the configuration file for third party dependencies that must be built
# standalone and isolated from rest of the project. The dependencies declared in
# this file are downloaded, configured and built at configuration time.
# =================================================================================================

cmake_minimum_required(VERSION 3.27.3)
project(eolo-external LANGUAGES C CXX)

find_package(Git REQUIRED)
include(ExternalProject)

# --------------------------------------------------------------------------------------------------
# Setup baseline build configuration
# --------------------------------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules/")
set_directory_properties(PROPERTIES EP_UPDATE_DISCONNECTED 1) # skip update step

# Collect common arguments to configure external projects
set(EP_CMAKE_EXTRA_ARGS
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_RPATH=${CMAKE_INSTALL_RPATH}
    -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
    -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
    -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD})

# helper macro useful for dependency handling
macro(add_dummy_target)
  if(NOT TARGET ${ARGV0})
    add_custom_target(${ARGV0})
  endif()
endmacro()

# --------------------------------------------------------------------------------------------------
# External projects follow
# --------------------------------------------------------------------------------------------------

# --------------------------------------------------------------------------------------------------
# zenohcxx
if(zenohcxx IN_LIST EXTERNAL_PROJECTS_LIST)
  # install zenoh-c first
  # set(zenohc_VERION_REQUIRED 0.11.0.0)
  find_package(zenohc QUIET) # ${zenohc_VERION_REQUIRED} QUIET)
  if(zenohc_FOUND)
    message(STATUS "zenohc: Using version ${zenohc_VERSION} from ${zenohc_DIR}")
    add_dummy_target(zenohc)
  else()
    message(STATUS "zenohc: Building ${zenohc_VERION_REQUIRED} from source")
    ExternalProject_Add(
      zenohc
      GIT_REPOSITORY "https://github.com/eclipse-zenoh/zenoh-c.git"
      GIT_TAG master # v${zenohc_VERION_REQUIRED}
      GIT_SHALLOW true
      CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS} -DCMAKE_VERBOSE_MAKEFILE=ON)
  endif()

  # set(zenohcxx_VERION_REQUIRED 0.11.0.0)
  find_package(zenohcxx QUIET) # ${zenohcxx_VERION_REQUIRED} QUIET)
  if(zenohcxx_FOUND)
    message(
      STATUS "zenohcxx: Using version ${zenohcxx_VERSION} from ${zenohcxx_DIR}")
    add_dummy_target(zenohcxx)
  else()
    message(STATUS "zenohcxx: Building ${zenohcxx_VERION_REQUIRED} from source")
    ExternalProject_Add(
      zenohcxx
      GIT_REPOSITORY "https://github.com/eclipse-zenoh/zenoh-cpp.git"
      GIT_TAG main # v${zenohcxx_VERION_REQUIRED}
      GIT_SHALLOW true
      SOURCE_SUBDIR install # configure the 'install' sub-project to avoid
                            # building zenoh-c again
      DEPENDS zenohc
      CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS})
  endif()
endif()
