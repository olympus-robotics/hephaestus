# =================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
# =================================================================================================

# =================================================================================================
# This is the configuration file for third party dependencies that must be built standalone and isolated from rest of
# the project. The dependencies declared in this file are downloaded, configured and built at configuration time.
# =================================================================================================

cmake_minimum_required(VERSION 3.27.3)
project(eolo-external LANGUAGES C CXX)

include(${CMAKE_TEMPLATE_DIR}/external.cmake)

# --------------------------------------------------------------------------------------------------
# For each target add the list of dependencies.
# This needs to be done at the beginning to make sure that the macro correctly includes them.
# --------------------------------------------------------------------------------------------------
if(zenohcxx IN_LIST EXTERNAL_PROJECTS_LIST)
  set(ZENOHCXX_DEPENDS zenohc zenohd)
  list(APPEND EXTERNAL_PROJECTS_LIST ${ZENOHCXX_DEPENDS})
endif()

if(Protobuf IN_LIST EXTERNAL_PROJECTS_LIST)
  set(PROTOBUF_DEPENDS absl)
  list(APPEND EXTERNAL_PROJECTS_LIST ${PROTOBUF_DEPENDS})
endif()

# --------------------------------------------------------------------------------------------------
# zenoh

if(zenohd IN_LIST EXTERNAL_PROJECTS_LIST)
  set(zenohd_VERION_REQUIRED 0.10.1-rc)
  if(EXISTS "${CMAKE_PREFIX_PATH}/bin/zenohd")
    message(STATUS "zenohd: Using from ${CMAKE_PREFIX_PATH}")
    add_dummy_target(zenohd)
  else()
    message(STATUS "zenohd: Building ${zenohd_VERION_REQUIRED} from source")
    set(ZENOH_PLUGIN_LIB "target/release/libzenoh_plugin_rest.so")
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      set(ZENOH_PLUGIN_LIB "target/release/libzenoh_plugin_rest.dylib")
    endif()
    ExternalProject_Add(
      zenohd
      GIT_REPOSITORY "https://github.com/eclipse-zenoh/zenoh.git"
      GIT_TAG ${zenohd_VERION_REQUIRED}
      GIT_SHALLOW true
      CONFIGURE_COMMAND ""
      BUILD_COMMAND
      COMMAND cargo build --release --package zenoh-plugin-rest --package zenohd
      BUILD_IN_SOURCE true
      INSTALL_COMMAND cp target/release/zenohd ${CMAKE_INSTALL_PREFIX}/bin/ && cp ${ZENOH_PLUGIN_LIB}
                      ${CMAKE_INSTALL_PREFIX}/bin/
      LOG_BUILD ON
    )
  endif()
endif()

add_cmake_dependency(
    NAME zenohc
    DEPENDS zenohd
    GIT_REPOSITORY "https://github.com/filippobrizzi/zenoh-c.git"
    GIT_TAG "df4682bd397723a62853dae62cdf65305c08958d"
)

add_cmake_dependency(
    NAME zenohcxx
    DEPENDS zenohc
    GIT_REPOSITORY "https://github.com/filippobrizzi/zenoh-cpp.git"
    GIT_TAG "457775eecc2b4fab15349464c373e59127896c28"
    SOURCE_SUBDIR install
)

# --------------------------------------------------------------------------------------------------
# Abseil C++
set(ABSEIL_VERSION 20230802)
set(ABSEIL_TAG ${ABSEIL_VERSION}.1)
set(ABSEIL_CMAKE_ARGS -DABSL_BUILD_TESTING=OFF -DABSL_ENABLE_INSTALL=ON -DABSL_PROPAGATE_CXX_STD=ON
                          -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
    )
add_cmake_dependency(
    NAME absl
    VERSION ${ABSEIL_VERSION}
    GIT_REPOSITORY "https://github.com/abseil/abseil-cpp.git"
    GIT_TAG ${ABSEIL_TAG}
    CMAKE_ARGS ${ABSEIL_CMAKE_ARGS}
)


# --------------------------------------------------------------------------------------------------
# protobuf
set(PROTOBUF_TAG v25.2)
set(PROTOBUF_VERSION 4.25.2)
set(PROTOBUF_CMAKE_ARGS -Dprotobuf_BUILD_EXAMPLES=OFF -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_ABSL_PROVIDER=package
                            -Dprotobuf_BUILD_CONFORMANCE=OFF
    )
add_cmake_dependency(
    NAME Protobuf
    DEPENDS ${PROTOBUF_DEPENDS}
    VERSION ${PROTOBUF_VERSION}
    GIT_REPOSITORY "https://github.com/protocolbuffers/protobuf.git"
    GIT_TAG ${PROTOBUF_TAG}
    CMAKE_ARGS ${PROTOBUF_CMAKE_ARGS}
)

# --------------------------------------------------------------------------------------------------
# Eigen
set(EIGEN_VERSION 3.4.0)
set(EIGEN_CMAKE_ARGS -DCMAKE_POLICY_DEFAULT_CMP0090:STRING=NEW)
add_cmake_dependency(
    NAME Eigen3
    VERSION ${EIGEN_VERSION}
    GIT_REPOSITORY "https://gitlab.com/libeigen/eigen.git"
    GIT_TAG ${EIGEN_VERSION}
    CMAKE_ARGS ${EIGEN_CMAKE_ARGS}
)

# --------------------------------------------------------------------------------------------------
# nlohmann json

set(NLOHMANN_JSON_VERSION 3.11.3)
set(NLOHMANN_JSON_TAG v${NLOHMANN_JSON_VERSION})
set(NLOHMANN_JSON_CMAKE_ARGS -DJSON_BuildTests=OFF)
add_cmake_dependency(
    NAME nlohmann_json
    VERSION ${NLOHMANN_JSON_VERSION}
    GIT_REPOSITORY "https://github.com/nlohmann/json.git"
    GIT_TAG ${NLOHMANN_JSON_TAG}
    CMAKE_ARGS ${NLOHMANN_JSON_CMAKE_ARGS}
)

# --------------------------------------------------------------------------------------------------
# magic_enum
set(MAGIC_ENUM_VERSION 0.9.5)
set(MAGIC_ENUM_TAG v${MAGIC_ENUM_VERSION})
set(MAGIC_ENUM_CMAKE_ARGS -DMAGIC_ENUM_OPT_BUILD_EXAMPLES=OFF -DMAGIC_ENUM_OPT_BUILD_TESTS=OFF
    -DMAGIC_ENUM_OPT_INSTALL=ON)
add_cmake_dependency(
    NAME magic_enum
    VERSION ${MAGIC_ENUM_VERSION}
    GIT_REPOSITORY "https://github.com/Neargye/magic_enum.git"
    GIT_TAG ${MAGIC_ENUM_TAG}
    CMAKE_ARGS ${MAGIC_ENUM_CMAKE_ARGS}
)

# --------------------------------------------------------------------------------------------------
# fmt
set(FMT_VERSION 10.2.1)
set(FMT_CMAKE_ARGS -DFMT_MASTER_PROJECT=OFF)
add_cmake_dependency(
    NAME fmt
    VERSION ${FMT_VERSION}
    GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
    GIT_TAG ${FMT_VERSION}
    CMAKE_ARGS ${FMT_CMAKE_ARGS}
)

# --------------------------------------------------------------------------------------------------
# range_v3
set(RANGE-V3_VERSION 0.12.0)
set(RANGE-V3_CMAKE_ARGS -DRANGE_V3_TESTS=OFF -DRANGE_V3_EXAMPLES=OFF -DRANGE_V3_DOCS=OFF)
add_cmake_dependency(
    NAME range-v3
    VERSION ${RANGE-V3_VERSION}
    GIT_REPOSITORY "https://github.com/ericniebler/range-v3.git"
    GIT_TAG ${RANGE-V3_VERSION}
    CMAKE_ARGS ${RANGE-V3_CMAKE_ARGS}
)
