# =================================================================================================
# Copyright (C) 2018 GRAPE Contributors Copyright (C) 2023-2024 EOLO Contributors
# =================================================================================================

macro(build_external_dependencies)
  set(single_opts FOLDER)

  include(CMakeParseArguments)
  cmake_parse_arguments(TARGET_ARG "${flags}" "${single_opts}" "${multi_opts}" ${ARGN})

  if(TARGET_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments: ${TARGET_ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT TARGET_ARG_FOLDER)
    message(FATAL_ERROR "Folder with external CMakeLists not specified")
  endif()

  # -------------------------------------------------------------------------------------------------
  # Build external dependencies before configuring project

  # define staging directories for external projects
  set(EP_BINARY_DIR ${CMAKE_BINARY_DIR}/external_${PROJECT_NAME}/) # location of build tree for external projects
  set(EP_DEPLOY_DIR ${CMAKE_BINARY_DIR}/external/deploy) # location of installable files from external projects
  file(MAKE_DIRECTORY ${EP_BINARY_DIR})
  file(MAKE_DIRECTORY ${EP_DEPLOY_DIR})
  get_property(_external_projects_list GLOBAL PROPERTY EXTERNAL_PROJECTS)

  # Because execute_process() cannot handle ';' character in a list when passed as argument, escape it
  string(REPLACE ";" "\\;" formatted_external_projects_list "${_external_projects_list}")

  # Configuration of all external dependencies are in external/CMakeLists.txt NOTE: external/CMakeLists.txt is processed
  # as if it was a separate project. This means: - Variables set there are not shared by the rest of this project -
  # CMake parameters must be explicitly passed as if cmake was called on it from the command line
  message(STATUS "========= External dependencies (from folder: ${TARGET_ARG_FOLDER}): Configuring =========")
  list(APPEND EXTERNAL_PREFIX_PATH ${CMAKE_INSTALL_PREFIX} ${CMAKE_PREFIX_PATH})
  execute_process(
    COMMAND
      ${CMAKE_COMMAND} -G "Ninja" ${TARGET_ARG_FOLDER} # Use 'Ninja' for parallel build
      -DEXTERNAL_PROJECTS_LIST=${formatted_external_projects_list} -DCMAKE_INSTALL_RPATH=${CMAKE_INSTALL_RPATH}
      -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_PREFIX_PATH=${EXTERNAL_PREFIX_PATH} -DCMAKE_INSTALL_PREFIX=${EP_DEPLOY_DIR}
      -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
      -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER} -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
      -DCMAKE_TEMPLATE_DIR=${CMAKE_TEMPLATES_DIR}
    WORKING_DIRECTORY ${EP_BINARY_DIR}
    RESULT_VARIABLE _result
  )

  # if configuration succeeded, then build all external projects
  if(${_result} EQUAL 0)
    message(STATUS "========= External dependencies: Building    =========")
    execute_process(
      COMMAND ${CMAKE_COMMAND} --build .
      WORKING_DIRECTORY ${EP_BINARY_DIR}
      RESULT_VARIABLE _result
    )
  endif()

  # If anything went wrong with external project build, stop and exit
  if(NOT ${_result} EQUAL 0)
    message(FATAL_ERROR "Error processing ${PROJECT_SOURCE_DIR}/external/CMakeLists.txt")
  endif()

  list(PREPEND CMAKE_PREFIX_PATH ${EP_DEPLOY_DIR}/)
  install(
    DIRECTORY ${EP_DEPLOY_DIR}/
    DESTINATION ${CMAKE_INSTALL_PREFIX}
    USE_SOURCE_PERMISSIONS
  )
  if(CMAKE_CROSSCOMPILING)
    list(APPEND CMAKE_FIND_ROOT_PATH ${EP_DEPLOY_DIR})
  endif()

  message(STATUS "======= External dependencies: Completed build =======")
endmacro()
