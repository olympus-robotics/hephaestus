# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

find_package(Git REQUIRED)
include(ExternalProject)

# --------------------------------------------------------------------------------------------------
# Setup baseline build configuration
# --------------------------------------------------------------------------------------------------
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
    -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
    -DBUILD_TESTING=OFF
)
list(PREPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})

make_directory(${CMAKE_INSTALL_PREFIX}/bin)
make_directory(${CMAKE_INSTALL_PREFIX}/lib)
make_directory(${CMAKE_INSTALL_PREFIX}/include)

message(STATUS "Dependencies")

# helper macro useful for dependency handling
macro(add_dummy_target)
  if(NOT TARGET ${ARGV0})
    add_custom_target(${ARGV0})
  endif()
endmacro()

macro(add_cmake_dependency)
  set(single_opts NAME VERSION URL GIT_TAG GIT_REPOSITORY SOURCE_SUBDIR)
  set(multi_opts CMAKE_ARGS DEPENDS)
  include(CMakeParseArguments)
  cmake_parse_arguments(TARGET_ARG "${flags}" "${single_opts}" "${multi_opts}" ${ARGN})

  if(TARGET_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments: ${TARGET_ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT TARGET_ARG_NAME)
    message(FATAL_ERROR "Library name not specified")
  endif()

  if(NOT TARGET_ARG_GIT_REPOSITORY AND NOT TARGET_ARG_URL)
    message(FATAL_ERROR "Specify one between GIT_REPOSITORY and URL")
  endif()

  if(TARGET_ARG_GIT_REPOSITORY AND TARGET_ARG_URL)
    message(FATAL_ERROR "Specify only one between GIT_REPOSITORY and URL")
  endif()

  if(TARGET_ARG_GIT_REPOSITORY AND NOT TARGET_ARG_GIT_TAG)
    message(FATAL_ERROR "Git tag not specified")
  endif()

  if(NOT TARGET_ARG_VERSION)
    set(TARGET_ARG_VERSION "")
  endif()

  if(NOT TARGET_ARG_CMAKE_ARGS)
    set(TARGET_ARG_CMAKE_ARGS "")
  endif()

  if(NOT TARGET_ARG_SOURCE_SUBDIR)
    set(TARGET_ARG_SOURCE_SUBDIR "./")
  endif()

  set(CMAKE_ARGS ${EP_CMAKE_EXTRA_ARGS})
  list(APPEND CMAKE_ARGS ${TARGET_ARG_CMAKE_ARGS})

  if(${TARGET_ARG_NAME} IN_LIST EXTERNAL_PROJECTS_LIST)
    find_package(${TARGET_ARG_NAME} ${TARGET_ARG_VERSION} QUIET)
    if(${TARGET_ARG_NAME}_FOUND)
      if(${TARGET_ARG_NAME}_DIR)
        set(TARGET_DIR ${${TARGET_ARG_NAME}_DIR})
      else()
        set(TARGET_DIR ${${TARGET_ARG_NAME}_LIBRARIES})
      endif()
      message(STATUS "    ${TARGET_ARG_NAME}: Using version ${TARGET_ARG_VERSION} from ${TARGET_DIR}")
      add_dummy_target(${TARGET_ARG_NAME})
    else()
      message(STATUS "    ${TARGET_ARG_NAME}: Building ${TARGET_ARG_VERSION} from source")
      if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.29")
        set(DOWNLOAD_EXTRACT_TIMESTAMP_ARG "DOWNLOAD_EXTRACT_TIMESTAMP;TRUE")
      else()
        set(DOWNLOAD_EXTRACT_TIMESTAMP_ARG "")
      endif()
      if(TARGET_ARG_GIT_REPOSITORY)
        ExternalProject_Add(
          ${TARGET_ARG_NAME}
          DEPENDS ${TARGET_ARG_DEPENDS}
          GIT_REPOSITORY ${TARGET_ARG_GIT_REPOSITORY}
          GIT_TAG ${TARGET_ARG_GIT_TAG}
          CMAKE_ARGS ${CMAKE_ARGS}
          GIT_SHALLOW true
          GIT_SUBMODULES_RECURSE false
          SOURCE_SUBDIR ${TARGET_ARG_SOURCE_SUBDIR}
          ${DOWNLOAD_EXTRACT_TIMESTAMP_ARG}
        )
      else()
        ExternalProject_Add(
          ${TARGET_ARG_NAME}
          DEPENDS ${TARGET_ARG_DEPENDS}
          URL ${TARGET_ARG_URL}
          CMAKE_ARGS ${CMAKE_ARGS}
          SOURCE_SUBDIR ${TARGET_ARG_SOURCE_SUBDIR}
          ${DOWNLOAD_EXTRACT_TIMESTAMP_ARG}
        )
      endif()
    endif()
  endif()
endmacro()
