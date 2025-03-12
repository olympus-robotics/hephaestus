# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

# =================================================================================================
# This is the configuration file for third party dependencies that must be built standalone and isolated from rest of
# the project. The dependencies declared in this file are downloaded, configured and built at configuration time.
# =================================================================================================

cmake_minimum_required(VERSION 3.22.1)
project(hephaestus-external LANGUAGES C CXX)

include(${CMAKE_TEMPLATE_DIR}/external.cmake)

# --------------------------------------------------------------------------------------------------
# For each target add the list of dependencies. This needs to be done at the beginning to make sure that the macro
# correctly includes them.
# --------------------------------------------------------------------------------------------------
if(zenohcxx IN_LIST EXTERNAL_PROJECTS_LIST)
  set(ZENOHCXX_DEPENDS zenohc)
  list(APPEND EXTERNAL_PROJECTS_LIST ${ZENOHCXX_DEPENDS})
endif()

if(Protobuf IN_LIST EXTERNAL_PROJECTS_LIST)
  set(PROTOBUF_DEPENDS absl)
  list(APPEND EXTERNAL_PROJECTS_LIST ${PROTOBUF_DEPENDS})
endif()

if(InfluxDB IN_LIST EXTERNAL_PROJECTS_LIST)
  set(INFLUXDB_DEPENDS cpr CURL)
  list(APPEND EXTERNAL_PROJECTS_LIST ${INFLUXDB_DEPENDS})
endif()

if(cpr IN_LIST EXTERNAL_PROJECTS_LIST)
  set(CPR_DEPENDS CURL)
  list(APPEND EXTERNAL_PROJECTS_LIST ${CPR_DEPENDS})
endif()

if(reflectcpp IN_LIST EXTERNAL_PROJECTS_LIST)
  set(REFLECT_CPP_DEPENDS yaml-cpp)
  list(APPEND EXTERNAL_PROJECTS_LIST ${REFLECT_CPP_DEPENDS})
endif()

if(foxglove_websocket IN_LIST EXTERNAL_PROJECTS_LIST)
  set(WS_PROTOCOL_DEPENDS nlohmann_json)
  list(APPEND EXTERNAL_PROJECTS_LIST ${WS_PROTOCOL_DEPENDS})
endif()

# --------------------------------------------------------------------------------------------------
# zenoh

set(ZENOHC_CMAKE_ARGS -DZENOHC_CARGO_CHANNEL=+stable -DZENOHC_BUILD_WITH_SHARED_MEMORY=TRUE
                      -DZENOHC_BUILD_WITH_UNSTABLE_API=TRUE
)
if(NOT BUILD_SHARED_LIBS)
  list(APPEND ZENOHC_CMAKE_ARGS "-DZENOHC_INSTALL_STATIC_LIBRARY=TRUE")
endif()
set(ZENOH_VERSION "release/1.2.1")
add_cmake_dependency(
  NAME zenohc
  GIT_REPOSITORY https://github.com/eclipse-zenoh/zenoh-c
  GIT_TAG ${ZENOH_VERSION}
  CMAKE_ARGS ${ZENOHC_CMAKE_ARGS}
)

set(ZENOHCXX_VERSION ${ZENOH_VERSION})
set(ZENOHCXX_CMAKE_ARGS -DZ_FEATURE_UNSTABLE_API=1, -DZENOH_WITH_UNSTABLE_API=1, -DZENOHCXX_ZENOHC=1,
                        -DZENOHCXX_EXAMPLES_PROTOBUF=OFF
)
add_cmake_dependency(
  NAME zenohcxx
  DEPENDS ${ZENOHCXX_DEPENDS} URL URL https://github.com/eclipse-zenoh/zenoh-cpp/archive/${ZENOHCXX_VERSION}.zip
  CMAKE_ARGS ${ZENOHCXX_CMAKE_ARGS}
)

# --------------------------------------------------------------------------------------------------
# Abseil C++
set(ABSEIL_VERSION 20240116)
set(ABSEIL_TAG ${ABSEIL_VERSION}.2)
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
set(PROTOBUF_TAG v27.2)
set(PROTOBUF_VERSION 5.27.2)
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
set(MAGIC_ENUM_VERSION 0.9.6)
set(MAGIC_ENUM_TAG v${MAGIC_ENUM_VERSION})
set(MAGIC_ENUM_CMAKE_ARGS -DMAGIC_ENUM_OPT_BUILD_EXAMPLES=OFF -DMAGIC_ENUM_OPT_BUILD_TESTS=OFF
                          -DMAGIC_ENUM_OPT_INSTALL=ON
)
add_cmake_dependency(
  NAME magic_enum
  VERSION ${MAGIC_ENUM_VERSION}
  GIT_REPOSITORY "https://github.com/Neargye/magic_enum.git"
  GIT_TAG ${MAGIC_ENUM_TAG}
  CMAKE_ARGS ${MAGIC_ENUM_CMAKE_ARGS}
)

# --------------------------------------------------------------------------------------------------
# fmt
set(FMT_VERSION 11.0.2)
set(FMT_CMAKE_ARGS -DFMT_MASTER_PROJECT=OFF)
add_cmake_dependency(
  NAME fmt
  VERSION ${FMT_VERSION}
  GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
  GIT_TAG ${FMT_VERSION}
  CMAKE_ARGS ${FMT_CMAKE_ARGS}
)

# --------------------------------------------------------------------------------------------------
# yaml-cpp
set(YAML_CPP_VERSION 0.8.0)
add_cmake_dependency(
  NAME yaml-cpp
  VERSION ${YAML_CPP_VERSION}
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG ${YAML_CPP_VERSION}
)

# --------------------------------------------------------------------------------------------------
# reflect-cpp
set(REFLECT_CPP_VERSION 0.16.0)
set(REFLECT_CPP_TAG v${REFLECT_CPP_VERSION}-cmake-3.22.1)
add_cmake_dependency(
  NAME reflectcpp
  DEPENDS ${REFLECT_CPP_DEPENDS}
  GIT_REPOSITORY https://github.com/olympus-robotics/reflect-cpp.git
  GIT_TAG ${REFLECT_CPP_TAG}
  CMAKE_ARGS -DREFLECTCPP_YAML=ON -DREFLECTCPP_JSON=ON -DCMAKE_BUILD_TYPE=Release
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

# --------------------------------------------------------------------------------------------------
# zstd
set(ZSTD_VERSION "1.5.6")
set(ZSTD_CMAKE_ARGS -DZSTD_BUILD_STATIC=ON -DZSTD_BUILD_SHARED=OFF -DZSTD_BUILD_TESTS=OFF -G Ninja)
add_cmake_dependency(
  NAME zstd
  VERSION ${ZSTD_VERSION}
          URL "https://github.com/facebook/zstd/releases/download/v${ZSTD_VERSION}/zstd-${ZSTD_VERSION}.tar.gz"
  SOURCE_SUBDIR build/cmake
  CMAKE_ARGS ${ZSTD_CMAKE_ARGS}
)

# --------------------------------------------------------------------------------------------------
# mcap
set(MCAP_TAG ff7ef1b941f4eb17c87a31b3ed5a55627f3a4aee)
set(MCAP_CMAKE_ARGS -DMCAP_COMPRESSION_NO_LZ4=ON)
add_cmake_dependency(
  NAME mcap
  VERSION ${MCAP_VERSION} URL https://github.com/olympus-robotics/mcap_builder/archive/${MCAP_TAG}.zip
  CMAKE_ARGS ${MCAP_CMAKE_ARGS}
)

# --------------------------------------------------------------------------------------------------
# curl
set(CURL_VERSION 8.7.0)
set(CURL_TAG curl-8_7_1)
add_cmake_dependency(
  NAME CURL
  VERSION ${CURL_VERSION}
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG ${CURL_TAG}
)

# --------------------------------------------------------------------------------------------------
# cpr (REST client)
set(CPR_GIT_TAG 21d3ad51afbd8e28931f8067cee3ff1ebd854a85)
add_cmake_dependency(
  NAME cpr
  DEPENDS ${CPR_DEPENDS} URL https://github.com/libcpr/cpr/archive/${CPR_GIT_TAG}.tar.gz
  CMAKE_ARGS -DCPR_USE_SYSTEM_CURL=ON
)

# --------------------------------------------------------------------------------------------------
# influxdb
set(INFLUXDB_GIT_TAG 2b8a87cfbb2b0a8c01a38212fade79553c8f75c8)
add_cmake_dependency(
  NAME InfluxDB
  DEPENDS ${INFLUXDB_DEPENDS} URL https://github.com/offa/influxdb-cxx/archive/${INFLUXDB_GIT_TAG}.zip
  CMAKE_ARGS -DINFLUXCXX_WITH_BOOST=OFF -DINFLUXCXX_TESTING=OFF -DINFLUXCXX_SYSTEMTEST=OFF
)

# --------------------------------------------------------------------------------------------------
# BoringSSL
set(BORINGSSL_VERSION "0.20250114.0")
set(BORINGSSL_TAG "chromium-stable")
add_cmake_dependency(
  NAME boringssl
  VERSION ${BORINGSSL_VERSION}
  GIT_REPOSITORY "https://boringssl.googlesource.com/boringssl"
  GIT_TAG ${BORINGSSL_TAG}
  CMAKE_ARGS -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -GNinja
)

# --------------------------------------------------------------------------------------------------
# ws-protocol
set(WS_PROTOCOL_VERSION 1.4.0)
set(WS_PROTOCOL_TAG releases/cpp/v${WS_PROTOCOL_VERSION}-dev)
add_cmake_dependency(
  NAME foxglove_websocket
  VERSION ${WS_PROTOCOL_VERSION}
  DEPENDS ${WS_PROTOCOL_DEPENDS}
  URL https://github.com/olympus-robotics/ws-protocol/archive/refs/tags/${WS_PROTOCOL_TAG}.zip
  GIT_TAG ${WS_PROTOCOL_TAG}
  SOURCE_SUBDIR cpp/foxglove-websocket
  CMAKE_ARGS -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -G Ninja
)

