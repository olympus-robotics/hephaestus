# =================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
# =================================================================================================

cmake_minimum_required(VERSION 3.27.3)
project(eolo LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Integrate the Eolo Build System to setup the project
include(${CMAKE_SOURCE_DIR}/cmake/build.cmake)
