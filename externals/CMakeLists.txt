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

string(REPLACE "|" ";" CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")

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
set(ZENOHCXX_CMAKE_ARGS -DZ_FEATURE_UNSTABLE_API=1 -DZENOH_WITH_UNSTABLE_API=1 -DZENOHCXX_ZENOHC=1
                        -DZENOHCXX_EXAMPLES_PROTOBUF=OFF
)
add_cmake_dependency(
  NAME zenohcxx
  DEPENDS ${ZENOHCXX_DEPENDS} URL https://github.com/eclipse-zenoh/zenoh-cpp/archive/${ZENOHCXX_VERSION}.zip
  CMAKE_ARGS ${ZENOHCXX_CMAKE_ARGS}
)

# --------------------------------------------------------------------------------------------------
# Abseil C++
set(ABSEIL_VERSION 20250127)
set(ABSEIL_TAG ${ABSEIL_VERSION}.0)
set(ABSEIL_CMAKE_ARGS
    -DABSL_BUILD_TESTING=OFF -DABSL_ENABLE_INSTALL=ON -DABSL_PROPAGATE_CXX_STD=ON
    -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON
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
set(PROTOBUF_TAG v30.0)
set(PROTOBUF_VERSION 6.30.0)
set(PROTOBUF_CMAKE_ARGS -Dprotobuf_BUILD_EXAMPLES=OFF -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_ABSL_PROVIDER=package
                        -Dprotobuf_BUILD_CONFORMANCE=OFF -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON
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
set(FMT_VERSION 11.2.0)
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
  CMAKE_ARGS -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)

# --------------------------------------------------------------------------------------------------
# reflect-cpp
set(REFLECT_CPP_VERSION 0.17.0)
set(REFLECT_CPP_TAG ${REFLECT_CPP_VERSION}-cmake-3.22.1-1)
add_cmake_dependency(
  NAME reflectcpp
  DEPENDS ${REFLECT_CPP_DEPENDS}
  GIT_REPOSITORY https://github.com/olympus-robotics/reflect-cpp.git
  GIT_TAG ${REFLECT_CPP_TAG}
  CMAKE_ARGS -DREFLECTCPP_YAML=ON -DREFLECTCPP_JSON=ON -DCMAKE_BUILD_TYPE=Release -DREFLECTCPP_USE_VCPKG=OFF
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
  DEPENDS ${WS_PROTOCOL_DEPENDS} URL
          https://github.com/olympus-robotics/ws-protocol/archive/refs/tags/${WS_PROTOCOL_TAG}.zip
  GIT_TAG ${WS_PROTOCOL_TAG}
  SOURCE_SUBDIR cpp/foxglove-websocket
  CMAKE_ARGS -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -G Ninja -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)

# --------------------------------------------------------------------------------------------------
# liburing
set(LIBURING_VERSION "2.5")
set(LIBURING_TAG "liburing-${LIBURING_VERSION}")
if(liburing IN_LIST EXTERNAL_PROJECTS_LIST)
  # NOTE: liburing must always be re-built from source as the pkgconfig files has absolute paths and not prefixed with
  # install prefix. find_package(PkgConfig REQUIRED) pkg_check_modules(LIBURING REQUIRED liburing) if(LIBURING_FOUND)
  # message(STATUS " liburing: Using version ${LIBURING_VERSION} from ${LIBURING_PREFIX}") add_dummy_target(liburing)
  # else() Build and install project
  message(STATUS "    liburing: Building ${LIBURING_VERSION} from source")
  ExternalProject_Add(
    liburing
    URL https://github.com/axboe/liburing/archive/refs/tags/${LIBURING_TAG}.tar.gz
    BUILD_IN_SOURCE 1
    USES_TERMINAL_BUILD TRUE # make it verbose
    CONFIGURE_COMMAND ./configure --prefix=${CMAKE_INSTALL_PREFIX} --cc=${CMAKE_C_COMPILER} --cxx=${CMAKE_CXX_COMPILER}
    # NOTE: the following is awful, but is the only way I found to avoid linking against the shared library.
    BUILD_COMMAND make -j && make install DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    INSTALL_COMMAND rm -rf ${CMAKE_INSTALL_PREFIX}/lib/liburing.so && rm -rf ${CMAKE_INSTALL_PREFIX}/lib/liburing.so.2
                    rm -rf ${CMAKE_INSTALL_PREFIX}/lib/liburing.so.2.5
  )
  # endif(LIBURING_FOUND)
endif()

# --------------------------------------------------------------------------------------------------
# stdexec
set(STDEXEC_VERSION "daf12bcc46438f88b3c78212363f7b6531338780")
add_cmake_dependency(NAME stdexec URL https://github.com/NVIDIA/stdexec/archive/${STDEXEC_VERSION}.tar.gz)

# --------------------------------------------------------------------------------------------------
# boost
if(Boost IN_LIST EXTERNAL_PROJECTS_LIST)
  set(BOOST_VERSION_REQUIRED 1.83.0)
  set(CMAKE_POLICY_DEFAULT_CMP0167 NEW)

  find_package(Boost ${BOOST_VERSION_REQUIRED} QUIET CONFIG)
  if(Boost_FOUND)
    message(STATUS "Boost: Using version ${Boost_VERSION} from ${Boost_DIR}")
    add_dummy_target(Boost)
  else()
    message(STATUS "Boost: Building Boost ${BOOST_VERSION_REQUIRED} from source")
    string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
    if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
      set(BOOST_TOOLSET gcc)
    elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
      set(BOOST_TOOLSET clang)
    endif()
    if(CMAKE_CROSSCOMPILING)
      set(BOOST_TOOLSET ${BOOST_TOOLSET}-${CMAKE_SYSTEM_PROCESSOR})
    else()
      set(BOOST_TOOLSET ${BOOST_TOOLSET})
    endif()
    set(BOOST_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}")
    set(BOOST_LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}")
    set(BOOST_BUILD_COMMAND
        ${CMAKE_COMMAND} -E env BOOST_BUILD_PATH=${CMAKE_CURRENT_SOURCE_DIR}/boost ./b2 install variant=release
        threading=multi toolset=${BOOST_TOOLSET} dll-path='\$ORIGIN:\$ORIGIN/../lib:${CMAKE_INSTALL_PREFIX}/bin'
        cxxflags=${BOOST_CXX_FLAGS} linkflags=${BOOST_LINK_FLAGS}
    )
    if(${BUILD_SHARED_LIBS})
      set(BOOST_BUILD_COMMAND ${BOOST_BUILD_COMMAND} link=shared)
    else()
      set(BOOST_BUILD_COMMAND ${BOOST_BUILD_COMMAND} link=static)
    endif()
    string(REPLACE "." "_" BOOST_DOWNLOAD_SUFFIX ${BOOST_VERSION_REQUIRED})

    # Build and install project
    ExternalProject_Add(
      Boost
      # Note: Use official tarball because github tarball won't work and recursive git cloning takes too long. Note:
      # Cannot use git mirror until (many) third party dependencies of Boost are mirrored too.
      URL https://archives.boost.io/release/${BOOST_VERSION_REQUIRED}/source/boost_${BOOST_DOWNLOAD_SUFFIX}.tar.gz
      BUILD_IN_SOURCE 1
      USES_TERMINAL_BUILD TRUE # make it verbose
      CONFIGURE_COMMAND
        ./bootstrap.sh --with-libraries=system
        --prefix=${CMAKE_INSTALL_PREFIX} # NOTE: we only require circular_buffer, system is a placeholder to have something to compile. Extend if you need more.
      BUILD_COMMAND ${BOOST_BUILD_COMMAND}
      INSTALL_COMMAND "" DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
  endif(Boost_FOUND)
endif()
