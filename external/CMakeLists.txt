# =================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
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

#--------------------------------------------------------------------------------------------------
# Setup additional dependencies for some projects
#--------------------------------------------------------------------------------------------------

if(Protobuf IN_LIST EXTERNAL_PROJECTS_LIST)
    set(PROTOBUF_DEPENDS absl)
    list(APPEND EXTERNAL_PROJECTS_LIST ${PROTOBUF_DEPENDS})
endif()

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
      DEPENDS zenohc
      GIT_REPOSITORY "https://github.com/eclipse-zenoh/zenoh-cpp.git"
      GIT_TAG main # v${zenohcxx_VERION_REQUIRED}
      GIT_SHALLOW true
      SOURCE_SUBDIR install # configure the 'install' sub-project to avoid
                            # building zenoh-c again
      DEPENDS zenohc
      CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS})
  endif()
endif()

#--------------------------------------------------------------------------------------------------
# Abseil C++
if (absl IN_LIST EXTERNAL_PROJECTS_LIST)
    set(ABSEIL_VERSION_REQUIRED 20240116)
    set(ABSEIL_PATCH_REQUIRED rc1)
    find_package(absl ${ABSEIL_VERSION_REQUIRED} QUIET)
    if (absl_FOUND)
        message(STATUS "Abseil: Using version ${absl_VERSION} from ${absl_DIR}")
        add_dummy_target(absl)
    else()
        message(STATUS "Abseil: Building ${ABSEIL_VERSION_REQUIRED}.${ABSEIL_PATCH_REQUIRED} from source")
        set(ABSEIL_CMAKE_ARGS
            -DABSL_BUILD_TESTING=OFF
            -DABSL_ENABLE_INSTALL=ON
            -DABSL_PROPAGATE_CXX_STD=ON
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD})
        ExternalProject_Add(
            absl
            URL https://github.com/abseil/abseil-cpp/archive/refs/tags/${ABSEIL_VERSION_REQUIRED}.${ABSEIL_PATCH_REQUIRED}.tar.gz
            URL_HASH MD5=4b774041b0f66bfbb95bfe3931d56296
            CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS} ${ABSEIL_CMAKE_ARGS})
    endif()
endif()

#--------------------------------------------------------------------------------------------------
# protobuf
if (Protobuf IN_LIST EXTERNAL_PROJECTS_LIST)
    set(PROTOBUF_VERSION_REQUIRED 25.2)
    find_package(Protobuf ${PROTOBUF_VERSION_REQUIRED} QUIET)
    if(Protobuf_FOUND)
        message(STATUS "Protobuf: Using version ${Protobuf_VERSION} (${Protobuf_LIBRARIES}, ${Protobuf_PROTOC_LIBRARIES})")
        add_dummy_target(Protobuf)
    else()
        message(STATUS "Protobuf: Building ${PROTOBUF_VERSION_REQUIRED} from source")
        ExternalProject_Add(
            Protobuf
            DEPENDS ${PROTOBUF_DEPENDS}
            GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf.git"
            GIT_TAG "v${PROTOBUF_VERSION_REQUIRED}"
            GIT_SHALLOW true
            GIT_SUBMODULES_RECURSE false
            CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS} -Dprotobuf_BUILD_EXAMPLES=OFF -Dprotobuf_BUILD_TESTS=OFF
        )
    endif()
endif()

#--------------------------------------------------------------------------------------------------
# Eigen
if(Eigen3 IN_LIST EXTERNAL_PROJECTS_LIST)
    set(EIGEN3_VERSION_REQUIRED 3.4.0)
    find_package(Eigen3 ${EIGEN3_VERSION_REQUIRED} QUIET)
    if(Eigen3_FOUND)
        message(STATUS "Eigen3: Using version ${Eigen3_VERSION} from ${Eigen3_DIR}")
        add_dummy_target(Eigen3)
    else()
        message(STATUS "Eigen3: Building ${EIGEN3_VERSION_REQUIRED} from source")
        ExternalProject_Add(
                Eigen3
                GIT_REPOSITORY "https://gitlab.com/libeigen/eigen.git"
                GIT_TAG ${EIGEN3_VERSION_REQUIRED}
                GIT_SHALLOW true
                CMAKE_ARGS -DCMAKE_POLICY_DEFAULT_CMP0090:STRING=NEW ${EP_CMAKE_EXTRA_ARGS})
    endif()
endif()
