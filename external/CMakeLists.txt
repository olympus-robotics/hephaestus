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

if(cpr IN_LIST EXTERNAL_PROJECTS_LIST)
  set(CPR_DEPENDS curl)
  list(APPEND EXTERNAL_PROJECTS_LIST ${CPR_DEPENDS})
endif()

if(influxdb IN_LIST EXTERNAL_PROJECTS_LIST)
  set(INFLUXDB_DEPENDS cpr)
  list(APPEND EXTERNAL_PROJECTS_LIST ${INFLUXDB_DEPENDS})
endif()

# --------------------------------------------------------------------------------------------------
# zenoh

set(ZENOH_VERSION "0.11.0.3")
add_cmake_dependency(
  NAME zenohc
  GIT_REPOSITORY "https://github.com/eclipse-zenoh/zenoh-c"
  GIT_TAG ${ZENOH_VERSION}
  CMAKE_ARGS -DZENOHC_CARGO_CHANNEL=+stable
)

add_cmake_dependency(
  NAME zenohcxx
  DEPENDS zenohc
  GIT_REPOSITORY "https://github.com/eclipse-zenoh/zenoh-cpp"
  GIT_TAG ${ZENOH_VERSION}
  SOURCE_SUBDIR install
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
set(MAGIC_ENUM_VERSION 0.9.5)
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

# --------------------------------------------------------------------------------------------------
# mcap
set(MCAP_TAG cb34e058cfd529f694a4ff1747517478339fc866)
add_cmake_dependency(
  NAME mcap VERSION ${MCAP_VERSION} URL https://github.com/olympus-robotics/mcap_builder/archive/${MCAP_TAG}.zip
)

# --------------------------------------------------------------------------------------------------
# curl (REST client)
set(CURL_VERSION 8.7.1)
set(CURL_TAG curl-8_7_1)
add_cmake_dependency(
  NAME curl
  VERSION ${CURL_VERSION}
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG ${CURL_TAG}
)

# --------------------------------------------------------------------------------------------------
# cpr (REST client)
set(CPR_GIT_TAG 21d3ad51afbd8e28931f8067cee3ff1ebd854a85)
add_cmake_dependency(
  NAME cpr
  DEPENDS curl
  URL https://github.com/libcpr/cpr/archive/${CPR_GIT_TAG}.tar.gz
  CMAKE_ARGS -DCPR_USE_SYSTEM_CURL=ON
)

# --------------------------------------------------------------------------------------------------
# influxdb (REST client)
set(INFLUXDB_GIT_TAG 8d8ab3b5ddc40b767a3ebf5eb1df76aa170dbca9)
add_cmake_dependency(
  NAME influxdb
  DEPENDS cpr
  URL https://github.com/offa/influxdb-cxx/archive/${INFLUXDB_GIT_TAG}.zip
  CMAKE_ARGS -DINFLUXCXX_WITH_BOOST=OFF -DINFLUXCXX_TESTING=OFF -DINFLUXCXX_SYSTEMTEST=OFF
)
