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
    set(ZENOH_PLUGIN_LIB "target/release/libzenoh_plugin_rest.so")
    if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(ZENOH_PLUGIN_LIB "target/release/libzenoh_plugin_rest.dylib")
    endif()
    ExternalProject_Add(
        zenohd
            GIT_REPOSITORY "https://github.com/eclipse-zenoh/zenoh.git"
            GIT_TAG  0.10.1-rc # v${zenohc_VERION_REQUIRED}
            GIT_SHALLOW true
            CONFIGURE_COMMAND ""
            BUILD_COMMAND COMMAND cargo build --release --package zenoh-plugin-rest --package zenohd
            BUILD_IN_SOURCE true
            INSTALL_COMMAND mkdir -p ${CMAKE_INSTALL_PREFIX}/bin/ && cp target/release/zenohd ${CMAKE_INSTALL_PREFIX}/bin/ && cp ${ZENOH_PLUGIN_LIB} ${CMAKE_INSTALL_PREFIX}/bin/
            LOG_BUILD ON)

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
        DEPENDS zenohd
        GIT_REPOSITORY "https://github.com/eclipse-zenoh/zenoh-c.git"
        GIT_TAG d1ec83dee421054bed034e0e60abc98a526bae0a # v${zenohc_VERION_REQUIRED}
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
        LOG_BUILD true
        CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS})

  endif()
endif()

#--------------------------------------------------------------------------------------------------
# Abseil C++
if (absl IN_LIST EXTERNAL_PROJECTS_LIST)
    set(ABSEIL_VERSION_REQUIRED 20230802.1)
    find_package(absl ${ABSEIL_VERSION_REQUIRED} QUIET)
    if (absl_FOUND)
        message(STATUS "Abseil: Using version ${absl_VERSION} from ${absl_DIR}")
        add_dummy_target(absl)
    else()
        message(STATUS "Abseil: Building ${ABSEIL_VERSION_REQUIRED} from source")
        set(ABSEIL_CMAKE_ARGS
            -DABSL_BUILD_TESTING=OFF
            -DABSL_ENABLE_INSTALL=ON
            -DABSL_PROPAGATE_CXX_STD=ON
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD})
        ExternalProject_Add(
            absl
                GIT_REPOSITORY https://github.com/abseil/abseil-cpp
                GIT_TAG ${ABSEIL_VERSION_REQUIRED}
                LOG_BUILD true
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
        set(PROTOBUF_CMAKE_ARGS
            -Dprotobuf_BUILD_EXAMPLES=OFF
            -Dprotobuf_BUILD_TESTS=OFF
            -Dprotobuf_ABSL_PROVIDER=package
            -Dprotobuf_BUILD_CONFORMANCE=OFF)
        ExternalProject_Add(
            Protobuf
                DEPENDS ${PROTOBUF_DEPENDS}
                GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf.git"
                GIT_TAG "v${PROTOBUF_VERSION_REQUIRED}"
                GIT_SHALLOW true
                GIT_SUBMODULES_RECURSE false
                CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS} ${PROTOBUF_CMAKE_ARGS}
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

#--------------------------------------------------------------------------------------------------
# nlohmann json
if(nlohmann_json IN_LIST EXTERNAL_PROJECTS_LIST)
    set(NLOHMANN_JSON_VERSION_REQUIRED 3.11.3)
    find_package(nlohmann_json ${NLOHMANN_JSON_VERSION_REQUIRED} QUIET)
    if(nlohmann_json_FOUND)
        message(STATUS "NLohmann JSON: Using version ${nlohmann_json_VERSION} from ${nlohmann_json_DIR}")
        add_dummy_target(nlohmann_json)
    else()
        message(STATUS "NLohmann JSON: Building ${NLOHMANN_JSON_VERSION_REQUIRED} from source")
        ExternalProject_Add(
            nlohmann_json
                GIT_REPOSITORY "https://github.com/nlohmann/json"
                GIT_TAG v${NLOHMANN_JSON_VERSION_REQUIRED}
                GIT_SHALLOW true
                CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS} -DJSON_BuildTests=OFF)
    endif()
endif()

#--------------------------------------------------------------------------------------------------
# magic_enum
if(magic_enum IN_LIST EXTERNAL_PROJECTS_LIST)
    set(MAGIC_ENUM_VERSION_REQUIRED 0.9.5)
    find_package(magic_enum ${MAGIC_ENUM_VERSION_REQUIRED} QUIET)
    if(magic_enum_FOUND)
        message(STATUS "Magic Enum: Using version ${magic_enum_VERSION} from ${magic_enum_DIR}")
        add_dummy_target(magic_enum)
    else()
        message(STATUS "Magic Enum: Building ${MAGIC_ENUM_VERSION_REQUIRED} from source")
        ExternalProject_Add(
            magic_enum
                GIT_REPOSITORY "https://github.com/Neargye/magic_enum.git"
                GIT_TAG v${MAGIC_ENUM_VERSION_REQUIRED}
                GIT_SHALLOW true
                CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS} -DMAGIC_ENUM_OPT_BUILD_EXAMPLES=OFF -DMAGIC_ENUM_OPT_BUILD_TESTS=OFF -DMAGIC_ENUM_OPT_INSTALL=ON)
    endif()
endif()

#--------------------------------------------------------------------------------------------------
# tabulate
if(tabulate IN_LIST EXTERNAL_PROJECTS_LIST)
    set(TABULATE_VERSION_REQUIRED 1.5)
    find_package(tabulate ${TABULATE_VERSION_REQUIRED} QUIET)
    if(tabulate_FOUND)
        message(STATUS "Tabulate: Using version ${tabulate_VERSION} from ${tabulate_DIR}")
        add_dummy_target(tabulate)
    else()
        message(STATUS "Tabulate: Building ${TABULATE_VERSION_REQUIRED} from source")
        ExternalProject_Add(
            tabulate
                GIT_REPOSITORY "https://github.com/p-ranav/tabulate.git"
                GIT_TAG v${TABULATE_VERSION_REQUIRED}
                GIT_SHALLOW true
                CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS} -DSAMPLES=OFF)
    endif()
endif()

#--------------------------------------------------------------------------------------------------
# fmt
if(fmt IN_LIST EXTERNAL_PROJECTS_LIST)
    set(FMT_VERSION_REQUIRED 10.2.1)
    find_package(fmt ${FMT_VERSION_REQUIRED} QUIET)
    if(fmt_FOUND)
        message(STATUS "fmt: Using version ${fmt_VERSION} from ${fmt_DIR}")
        add_dummy_target(fmt)
    else()
        message(STATUS "fmt: Building ${FMT_VERSION_REQUIRED} from source")
        ExternalProject_Add(
            fmt
                GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
                GIT_TAG ${FMT_VERSION_REQUIRED}
                GIT_SHALLOW true
                CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS} -DFMT_MASTER_PROJECT=OFF)
    endif()
endif()

#--------------------------------------------------------------------------------------------------
# range_v3
if(range-v3 IN_LIST EXTERNAL_PROJECTS_LIST)
    set(RANGE_V3_VERSION_REQUIRED 0.12.0)
    find_package(range-v3 ${RANGE_V3_VERSION_REQUIRED} QUIET)
    if(RANGE_V3_FOUND)
        message(STATUS "range-v3: Using ${RANGE_V3_DIR}")
    else()
        message(STATUS "range_v3: Building ${RANGE_V3_VERSION_REQUIRED} from source")
        ExternalProject_Add(
            range_v3
                GIT_REPOSITORY "https://github.com/ericniebler/range-v3.git"
		        GIT_TAG ${RANGE_V3_VERSION_REQUIRED}
                GIT_SHALLOW true
                 CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS}
                -DRANGE_V3_TESTS=OFF
                -DRANGE_V3_EXAMPLES=OFF
                -DRANGE_V3_DOCS=OFF
        )
    endif()
endif()
