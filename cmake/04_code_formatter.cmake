# =================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
# =================================================================================================

# --------------------------------------------------------------------------------------------------
# Configures source code formatting tools Apply auto-formatting by calling add_clang_format() on target
# --------------------------------------------------------------------------------------------------
option(ENABLE_FORMATTER "Enable automatic source file formatting with clang-format" ON)
option(FORMAT_FAIL_ON_CHANGE "Fail build if format causes formatting changes" OFF)

if(ENABLE_FORMATTER)
  find_program(CLANG_FORMAT_BIN NAMES clang-format QUIET)
  if(NOT CLANG_FORMAT_BIN)
    message(WARNING "Code formatter (clang-format) requested but not found")
  endif()
endif()

# print summary
message(STATUS "Auto-format code (ENABLE_FORMATTER): ${ENABLE_FORMATTER} (${CLANG_FORMAT_BIN})")
message(STATUS "Fail if code not formatted (FORMAT_FAIL_ON_CHANGE): ${FORMAT_FAIL_ON_CHANGE}")

# Target to run clang-format on all files without having to compile them
add_custom_target(format)

# Setup format for cmake files
if(ENABLE_FORMATTER)
  find_program(CMAKE_FORMAT_BIN NAMES cmake-format QUIET)
  if(NOT CMAKE_FORMAT_BIN)
    message(WARNING "cmake-format requested but not found")
  endif()
endif()

if(CMAKE_FORMAT_BIN AND ENABLE_FORMATTER)
  set(_cmake_files
      "${CMAKE_SOURCE_DIR}/CMakeLists.txt;${CMAKE_SOURCE_DIR}/external/CMakeLists.txt;${CMAKE_SOURCE_DIR}/cmake/*.cmake;${CMAKE_SOURCE_DIR}/modules/**/*.txt;toolchains/*.cmake"
  )
  if(FORMAT_FAIL_ON_CHANGE)
    add_custom_command(
      TARGET format
      PRE_BUILD
      COMMAND ${CMAKE_FORMAT_BIN} --check ${_cmake_files}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
  else()
    add_custom_command(
      TARGET format
      PRE_BUILD
      COMMAND ${CMAKE_FORMAT_BIN} -i ${_cmake_files}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
  endif()
endif()

# Function to apply clang formatting on a target
# https://www.linkedin.com/pulse/simple-elegant-wrong-how-integrate-clang-format-friends-brendan-drew

function(add_clang_format _targetname)
  if(CLANG_FORMAT_BIN AND ENABLE_FORMATTER)
    if(NOT TARGET ${_targetname})
      message(FATAL_ERROR "add_clangformat should only be called on targets (got \"${_targetname}\")")
    endif()

    # figure out which sources this should be applied to
    get_target_property(_clang_sources ${_targetname} SOURCES)
    get_target_property(_builddir ${_targetname} BINARY_DIR)

    # There are file types we don't want to process (or may be the inverse list is shorter)
    set(_supported_file_types ".c" ".cpp" ".h" ".hpp" ".inl")

    set(_sources "")
    foreach(_source ${_clang_sources})
      if(NOT TARGET ${_source})
        get_filename_component(_ext_type ${_source} EXT)
        if(NOT ${_ext_type} STREQUAL "")
          if(${_ext_type} IN_LIST _supported_file_types)
            get_filename_component(_source_file ${_source} NAME)
            get_source_file_property(_clang_loc "${_source}" LOCATION)

            math(EXPR counter "${counter}+1")
            set(_format_file .${CMAKE_FILES_DIRECTORY}/${_targetname}_${counter}_${_source_file}.format)
            if(FORMAT_FAIL_ON_CHANGE)
              add_custom_command(
                OUTPUT ${_format_file}
                DEPENDS ${_source}
                COMMAND ${CLANG_FORMAT_BIN} --dry-run --Werror ${_clang_loc}
                COMMAND ${CMAKE_COMMAND} -E touch ${_format_file}
              )
            else()
              add_custom_command(
                OUTPUT ${_format_file}
                DEPENDS ${_source}
                COMMENT "Format ${_source}"
                COMMAND ${CLANG_FORMAT_BIN} -style=file -i ${_clang_loc}
                COMMAND ${CMAKE_COMMAND} -E touch ${_format_file}
              )
            endif()
            list(APPEND _sources ${_format_file})
          endif() # in included types list
        endif() # extension detected
      endif()
    endforeach()

    if(_sources)
      add_custom_target(
        ${_targetname}_clangformat
        SOURCES ${_sources}
        COMMENT "Format target ${_target}"
      )
      add_dependencies(${_targetname} ${_targetname}_clangformat)
      add_dependencies(format ${_targetname}_clangformat)
    endif()
  endif() # CLANG_FORMAT_BIN found
endfunction()
