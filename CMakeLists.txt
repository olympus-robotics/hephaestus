# =================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
# =================================================================================================

cmake_minimum_required(VERSION 3.27.3)
project(eolo LANGUAGES CXX C)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(BUILD_AS_SUBPROJECT FALSE)
else()
  set(BUILD_AS_SUBPROJECT TRUE)
endif()

if(NOT BUILD_AS_SUBPROJECT)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Integrate the Eolo Build System to setup the project
include(${PROJECT_SOURCE_DIR}/cmake/build.cmake)
