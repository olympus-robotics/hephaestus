# ==================================================================================================
# Copyright (C) 2018 GRAPE Contributors Copyright (C) 2023-2024 EOLO Contributors
# ==================================================================================================

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(FetchContent)
include(CTest)

# ==================================================================================================
# Global list of all modules
define_property(
  GLOBAL
  PROPERTY DECLARED_MODULES
  INHERITED
  BRIEF_DOCS "All the declared modules in the system"
  FULL_DOCS "All the declared modules in the system"
)
set_property(GLOBAL PROPERTY DECLARED_MODULES "")

# Global list of all modules marked for build
define_property(
  GLOBAL
  PROPERTY ENABLED_MODULES
  INHERITED
  BRIEF_DOCS "Modules marked for build either directly or to satisfy dependencies"
  FULL_DOCS "Modules marked for build either directly or to satisfy dependencies"
)
set_property(GLOBAL PROPERTY ENABLED_MODULES "")

# Global list of external projects to build
define_property(
  GLOBAL
  PROPERTY EXTERNAL_PROJECTS
  INHERITED
  BRIEF_DOCS "External projects marked for build either directly or to satisfy dependencies"
  FULL_DOCS "External projects marked for build either directly or to satisfy dependencies"
)
set_property(GLOBAL PROPERTY EXTERNAL_PROJECTS "")

# ==================================================================================================
# Function: enumerate_modules
#
# Description: Recurses through the source tree and enumerate modules to build.
#
# Parameters: ROOT_PATH: Top level path to begin enumeration from
#
function(enumerate_modules)
  set(flags "")
  set(single_opts ROOT_PATH)
  set(multi_opts "")
  include(CMakeParseArguments)

  cmake_parse_arguments(_ARG "${flags}" "${single_opts}" "${multi_opts}" ${ARGN})

  if(_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments: ${_ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT _ARG_ROOT_PATH)
    message(FATAL_ERROR "ROOT_PATH not specified.")
  endif()

  # Run through all module files collecting information about them
  set(_module_paths)
  find_module_declarations(_module_paths ROOT_PATH ${_ARG_ROOT_PATH})
  set(_MODULES_ENUMERATE_FLAG
      ON
      CACHE INTERNAL "Enumeration of modules in progress"
  )
  message(STATUS "Enumerating ${PROJID} modules")
  foreach(_module IN LISTS _module_paths)
    if(NOT ${_module} STREQUAL ${CMAKE_CURRENT_LIST_FILE}) # avoid recursion
      message(VERBOSE "  Parsing ${_module}")
      include(${_module})
    endif()
  endforeach()
  set(_MODULES_ENUMERATE_FLAG
      OFF
      CACHE INTERNAL ""
  )

  # Go through list of enabled modules and mark it and its dependencies to be built
  mark_modules_to_build()

  # print a summary of modules
  get_property(_declared_modules_list GLOBAL PROPERTY DECLARED_MODULES)
  get_property(_enabled_modules_list GLOBAL PROPERTY ENABLED_MODULES)
  message(STATUS "Modules")
  foreach(_module IN LISTS _declared_modules_list)
    set(_reason_to_build "")
    if(BUILD_MODULE_${_module})
      set(_reason_to_build "Enabled")
    else()
      if(_module IN_LIST _enabled_modules_list)
        set(_reason_to_build "Enabled (dependency)")
      else()
        set(_reason_to_build "Disabled")
      endif()
    endif()
    message(STATUS "\t${_module}: ${_reason_to_build}")
  endforeach()
endfunction()

# ==================================================================================================
# macro: configure_modules
#
# Description: Configures modules that have been marked to be built during enumeration. See enumerate_modules()
#
macro(configure_modules)
  get_property(_enabled_modules_list GLOBAL PROPERTY ENABLED_MODULES)

  # Add each marked module into the build
  foreach(_module IN LISTS _enabled_modules_list)
    message(VERBOSE "Adding subdirectory ${MODULE_${_module}_PATH} for module ${_module}")
    add_subdirectory(${MODULE_${_module}_PATH})
  endforeach()

  # Generate input paths for source code documentation DOC_INPUT_PATHS DOC_EXAMPLE_PATHS
  set(DOC_INPUT_PATHS ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/docs ${CMAKE_SOURCE_DIR}/README.md)
  foreach(_module IN LISTS _enabled_modules_list)
    set(DOC_INPUT_PATHS ${DOC_INPUT_PATHS} ${MODULE_${_module}_PATH}/include ${MODULE_${_module}_PATH}/docs
                        ${MODULE_${_module}_PATH}/README.md
    )
    set(DOC_EXAMPLE_PATHS ${DOC_EXAMPLE_PATHS} ${MODULE_${_module}_PATH}/examples)
  endforeach()
  string(REPLACE ";" " " DOC_INPUT_PATHS "${DOC_INPUT_PATHS}")
  string(REPLACE ";" " " DOC_EXAMPLE_PATHS "${DOC_EXAMPLE_PATHS}")

  # Create doxygen configuration for source code documentation generation
  configure_file(${CMAKE_TEMPLATES_DIR}/doxyfile.in ${CMAKE_BINARY_DIR}/doxyfile @ONLY)

  # Define installation rules for module targets
  install_modules()
endmacro()

# ==================================================================================================
# macro: declare_module
#
# Description: Declares a module.
#
# Parameters: NAME (required): A unique name for the module ALWAYS_BUILD (optional): If true, the module is always
# built, disregarding BUILD_MODULES setting DEPENDS_ON_MODULES (list): List of other modules that this module depends on
# DEPENDS_ON_EXTERNAL_PROJECTS (list): List of tags to external projects that this module depends on
#
macro(declare_module)
  set(flags ALWAYS_BUILD EXCLUDE_FROM_ALL)
  set(single_opts NAME)
  set(multi_opts DEPENDS_ON_MODULES DEPENDS_ON_EXTERNAL_PROJECTS)

  include(CMakeParseArguments)
  cmake_parse_arguments(MODULE_ARG "${flags}" "${single_opts}" "${multi_opts}" ${ARGN})

  if(MODULE_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments: ${MODULE_ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT MODULE_ARG_NAME)
    message(FATAL_ERROR "NAME not specified")
  endif()

  # Either have it always build, or allow user to choose at configuration-time
  if(("${MODULE_ARG_NAME}" IN_LIST BUILD_MODULES)
     OR (("all" IN_LIST BUILD_MODULES) AND NOT MODULE_ARG_EXCLUDE_FROM_ALL)
     OR MODULE_ARG_ALWAYS_BUILD
  )
    # Explicitly disable modules
    if((NOT MODULE_ARG_ALWAYS_BUILD)
       AND DISABLE_MODULES
       AND "${MODULE_ARG_NAME}" IN_LIST DISABLE_MODULES
    )
      message("${MODULE_ARG_NAME} explicitly disabled")
    else()
      set(BUILD_MODULE_${MODULE_ARG_NAME} TRUE)
    endif()
  endif()

  # if we are just enumerating modules, then set module meta information and exit. create global variables:
  # DECLARED_MODULES MODULE_<module>_PATH MODULE_<module>_DEPENDS_ON
  if(_MODULES_ENUMERATE_FLAG)
    get_property(_declared_modules_list GLOBAL PROPERTY DECLARED_MODULES)
    if(${MODULE_ARG_NAME} IN_LIST _declared_modules_list)
      message(FATAL_ERROR "Another module named \"${MODULE_ARG_NAME}\" exists in ${MODULE_${MODULE_ARG_NAME}_PATH}")
    endif()
    set_property(GLOBAL APPEND PROPERTY DECLARED_MODULES ${MODULE_ARG_NAME})
    set(MODULE_${MODULE_ARG_NAME}_PATH
        "${CMAKE_CURRENT_LIST_DIR}"
        CACHE INTERNAL "location of ${MODULE_ARG_NAME}"
    )
    set(MODULE_${MODULE_ARG_NAME}_DEPENDS_ON
        ${MODULE_ARG_DEPENDS_ON_MODULES}
        CACHE INTERNAL "Dependencies of ${MODULE_ARG_NAME}"
    )
    set(MODULE_${MODULE_ARG_NAME}_EXTERNAL_PROJECT_DEPS
        ${MODULE_ARG_DEPENDS_ON_EXTERNAL_PROJECTS}
        CACHE INTERNAL "External project dependencies of ${MODULE_ARG_NAME}"
    )

    # NOTE: Don't process the rest of the file in which this macro was called
    return()
  endif()

  # ----------------------------------------------------
  # NOTE: Now we are in the context of the current module (for instance, due to call to add_subdirectory(this))
  # ----------------------------------------------------

  # Create a few variables available in the context of the currently executing module - MODULE_NAME : Equivalent to
  # PROJECT_NAME but for modules - MODULE_SOURCE_DIR   : Equivalent to PROJECT_SOURCE_DIR but for modules
  set(MODULE_NAME ${MODULE_ARG_NAME})
  set(MODULE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})
  message(VERBOSE "Configuring module \"${MODULE_NAME}\" in ${MODULE_SOURCE_DIR}")

  # The following are populated by define_module_library
  set(MODULE_${MODULE_NAME}_LIB_TARGETS
      ""
      CACHE INTERNAL "Library targets in module ${MODULE_NAME}"
  )
  set(MODULE_${MODULE_NAME}_EXE_TARGETS
      ""
      CACHE INTERNAL "Executable targets in module ${MODULE_NAME}"
  )
endmacro()

# ==================================================================================================
# Macro: define_module_library
#
# Description: Macro wraps CMake add_library and related functions/macros to generate libraries in a standardised
# manner. Specifically: - Creates library <project_name>_<library_name> - Creates alias name
# <project_name>::<library_name> - Note: A library target belongs to the enclosing module - Note: A module can have
# multiple libraries
#
# Parameters: NAME        : (string) Base name of the library. Eg: 'mylib' creates 'lib<project_name>_mylib.so/.a`
# SOURCES     : (list) Source files to compile into the library above NOINSTALL   : (optional) Flag to tell the build
# system not to install the library on call to `make install`. Useful for libraries only usable in the build tree (Eg:
# helper libs for tests and examples) [PUBLIC_INCLUDE_PATHS] : (list, optional) Publicly included directories. See cmake
# documentation for 'PUBLIC' keyword in `target_include_directories` [PRIVATE_INCLUDE_PATHS] : (list, optional)
# Privately included directories. See cmake documentation for 'PRIVATE' keyword in `target_include_directories`
# [SYSTEM_PUBLIC_INCLUDE_PATHS] : (list, optional) Publicly included system directories on some platforms. See 'SYSTEM'
# keyword in `target_include_directories` [SYSTEM_PRIVATE_INCLUDE_PATHS] : (list, optional) Privately included system
# directories on some platforms. See 'SYSTEM' keyword in `target_include_directories` [PUBLIC_LINK_LIBS] : (list,
# optional) Public link dependencies. See 'PUBLIC' keyword in `target_link_libraries` [PRIVATE_LINK_LIBS]: (list,
# optional) Private link dependencies. See 'PRIVATE' keyword in `target_link_libraries`
#
macro(define_module_library)
  set(flags NOINSTALL)
  set(single_opts NAME)
  set(multi_opts
      SOURCES
      PUBLIC_INCLUDE_PATHS
      PRIVATE_INCLUDE_PATHS
      SYSTEM_PUBLIC_INCLUDE_PATHS
      SYSTEM_PRIVATE_INCLUDE_PATHS
      PUBLIC_LINK_LIBS
      PRIVATE_LINK_LIBS
  )

  include(CMakeParseArguments)
  cmake_parse_arguments(TARGET_ARG "${flags}" "${single_opts}" "${multi_opts}" ${ARGN})

  if(TARGET_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments: ${TARGET_ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT TARGET_ARG_NAME)
    message(FATAL_ERROR "Library name not specified")
  endif()

  set(LIBRARY_NAME ${CMAKE_PROJECT_NAME}_${TARGET_ARG_NAME})
  set(LIBRARY_EXPORT_NAME ${TARGET_ARG_NAME})
  set(LIBRARY_NAME_ALIAS ${CMAKE_PROJECT_NAME}::${TARGET_ARG_NAME})

  add_library(${LIBRARY_NAME} ${TARGET_ARG_SOURCES})
  add_library(${LIBRARY_NAME_ALIAS} ALIAS ${LIBRARY_NAME})
  add_clang_format(${LIBRARY_NAME})

  if(NOT NOINSTALL)
    set(MODULE_${MODULE_NAME}_LIB_TARGETS
        ${MODULE_${MODULE_NAME}_LIB_TARGETS} ${LIBRARY_NAME}
        CACHE INTERNAL "Library targets in module ${MODULE_NAME}"
    )
  endif()

  target_include_directories(
    ${LIBRARY_NAME} BEFORE
    PUBLIC ${TARGET_ARG_PUBLIC_INCLUDE_PATHS}
    PRIVATE ${TARGET_ARG_PRIVATE_INCLUDE_PATHS}
  )

  target_include_directories(
    ${LIBRARY_NAME} SYSTEM BEFORE
    PUBLIC ${TARGET_ARG_SYSTEM_PUBLIC_INCLUDE_PATHS}
    PRIVATE ${TARGET_ARG_SYSTEM_PRIVATE_INCLUDE_PATHS}
  )

  target_link_libraries(
    ${LIBRARY_NAME}
    PUBLIC ${TARGET_ARG_PUBLIC_LINK_LIBS}
    PRIVATE ${TARGET_ARG_PRIVATE_LINK_LIBS}
  )

  set_target_properties(
    ${LIBRARY_NAME}
    PROPERTIES VERSION ${VERSION}
               SOVERSION ${VERSION_MAJOR}
               EXPORT_NAME ${LIBRARY_EXPORT_NAME}
  )

  # sometimes libraries have no cpp files. So this is needed
  set_target_properties(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

endmacro()

# ==================================================================================================
# Adds a custom target to group all example programs built on call to `make examples`. See `define_module_example`
add_custom_target(examples COMMENT "Building examples")

# ==================================================================================================
# macro: define_module_example
#
# Description: Macro to define an example program that is built on call to `make examples`. Note: - This is essentially
# a convenient wrapper over CMake `add_executable`. - Examples are not installed on call to `make install` - Examples
# automatically link to libraries in the enclosing module - A module can have multiple examples
#
# Parameters: SOURCES     : (list) Source files to compile [PUBLIC_INCLUDE_PATHS] : (list, optional) Publicly included
# directories. See cmake documentation for 'PUBLIC' keyword in `target_include_directories` [PRIVATE_INCLUDE_PATHS] :
# (list, optional) Privately included directories. See cmake documentation for 'PRIVATE' keyword in
# `target_include_directories` [SYSTEM_PUBLIC_INCLUDE_PATHS] : (list, optional) Publicly included system directories on
# some platforms. See 'SYSTEM' keyword in `target_include_directories` [SYSTEM_PRIVATE_INCLUDE_PATHS] : (list, optional)
# Privately included system directories on some platforms. See 'SYSTEM' keyword in `target_include_directories`
# [PUBLIC_LINK_LIBS] : (list, optional) Public link dependencies. See 'PUBLIC' keyword in `target_link_libraries`
# [PRIVATE_LINK_LIBS]: (list, optional) Private link dependencies. See 'PRIVATE' keyword in `target_link_libraries`
#
macro(define_module_example)
  set(flags "")
  set(single_opts NAME)
  set(multi_opts SOURCES PUBLIC_INCLUDE_PATHS PRIVATE_INCLUDE_PATHS PUBLIC_LINK_LIBS PRIVATE_LINK_LIBS)

  include(CMakeParseArguments)
  cmake_parse_arguments(TARGET_ARG "${flags}" "${single_opts}" "${multi_opts}" ${ARGN})

  if(TARGET_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments: ${TARGET_ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT TARGET_ARG_NAME)
    message(FATAL_ERROR "Executable name not specified")
  endif()

  set(TARGET_NAME ${CMAKE_PROJECT_NAME}_${MODULE_NAME}_${TARGET_ARG_NAME})

  add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL ${TARGET_ARG_SOURCES})
  add_clang_format(${TARGET_NAME})
  add_dependencies(examples ${TARGET_NAME}) # Set this example to be built on `make examples`

  target_include_directories(
    ${TARGET_NAME} BEFORE
    PUBLIC ${TARGET_ARG_PUBLIC_INCLUDE_PATHS}
    PRIVATE ${TARGET_ARG_PRIVATE_INCLUDE_PATHS}
  )

  target_link_libraries(
    ${TARGET_NAME}
    PUBLIC ${TARGET_ARG_PUBLIC_LINK_LIBS}
    PRIVATE ${MODULE_${MODULE_NAME}_LIB_TARGETS} # link to libraries from the enclosing module
            ${TARGET_ARG_PRIVATE_LINK_LIBS}
  )

endmacro()

# ==================================================================================================
# macro: define_module_executable
#
# Description: Macro to define exectuable targets Note: - An executable target belongs to the enclosing module - A
# module can have multiple executables - Executables are installed on call to `make install`
#
# Parameters: SOURCES     : (list) Source files to compile [PUBLIC_INCLUDE_PATHS] : (list, optional) Publicly included
# directories. See cmake documentation for 'PUBLIC' keyword in `target_include_directories` [PRIVATE_INCLUDE_PATHS] :
# (list, optional) Privately included directories. See cmake documentation for 'PRIVATE' keyword in
# `target_include_directories` [SYSTEM_PUBLIC_INCLUDE_PATHS] : (list, optional) Publicly included system directories on
# some platforms. See 'SYSTEM' keyword in `target_include_directories` [SYSTEM_PRIVATE_INCLUDE_PATHS] : (list, optional)
# Privately included system directories on some platforms. See 'SYSTEM' keyword in `target_include_directories`
# [PUBLIC_LINK_LIBS] : (list, optional) Public link dependencies. See 'PUBLIC' keyword in `target_link_libraries`
# [PRIVATE_LINK_LIBS]: (list, optional) Private link dependencies. See 'PRIVATE' keyword in `target_link_libraries`
#
macro(define_module_executable)
  set(flags "")
  set(single_opts NAME)
  set(multi_opts SOURCES PUBLIC_INCLUDE_PATHS PRIVATE_INCLUDE_PATHS PUBLIC_LINK_LIBS PRIVATE_LINK_LIBS)

  include(CMakeParseArguments)
  cmake_parse_arguments(TARGET_ARG "${flags}" "${single_opts}" "${multi_opts}" ${ARGN})

  if(TARGET_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments: ${TARGET_ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT TARGET_ARG_NAME)
    message(FATAL_ERROR "Executable name not specified")
  endif()

  set(TARGET_NAME ${CMAKE_PROJECT_NAME}_${TARGET_ARG_NAME})

  add_executable(${TARGET_NAME} ${TARGET_ARG_SOURCES})
  add_clang_format(${TARGET_NAME})

  set(MODULE_${MODULE_NAME}_EXE_TARGETS
      ${MODULE_${MODULE_NAME}_EXE_TARGETS} ${TARGET_NAME}
      CACHE INTERNAL "Targets in module ${MODULE_NAME}"
  )

  target_include_directories(
    ${TARGET_NAME} BEFORE
    PUBLIC ${TARGET_ARG_PUBLIC_INCLUDE_PATHS}
    PRIVATE ${TARGET_ARG_PRIVATE_INCLUDE_PATHS}
  )

  target_link_libraries(
    ${TARGET_NAME}
    PUBLIC ${TARGET_ARG_PUBLIC_LINK_LIBS}
    PRIVATE ${MODULE_${MODULE_NAME}_LIB_TARGETS} # link to libraries from the enclosing module
            ${TARGET_ARG_PRIVATE_LINK_LIBS}
  )

endmacro()

# --------------------------------------------------------------------------------------------------
# A macro to generate eolo library target from .proto files NOTE A proto library name must end with _proto suffix.
#
# Usage example: declare_module(NAME my_proto_module DEPENDS_ON_EXTERNAL_PROJECTS Protobuf # MUST be present )
#
# # No need to add the `find_package(Protobuf)` as it's done implicitly by `define_module_proto_library`.
#
# define_module_proto_library( NAME example_proto SOURCES messages_one.proto messages_two.proto PUBLIC_LINK_LIBS
# eolo_example_dependencies_proto  # Module containing proto files used by this module. )
# --------------------------------------------------------------------------------------------------
macro(define_module_proto_library)
  set(flags "")
  set(single_opts NAME)
  set(multi_opts SOURCES PUBLIC_LINK_LIBS PRIVATE_LINK_LIBS)

  include(CMakeParseArguments)
  cmake_parse_arguments(TARGET_ARG "${flags}" "${single_opts}" "${multi_opts}" ${ARGN})

  if(TARGET_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments: ${TARGET_ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT TARGET_ARG_NAME)
    message(FATAL_ERROR "Library name not specified")
  endif()

  if(NOT ${TARGET_ARG_NAME} MATCHES "_proto$")
    message(FATAL_ERROR "Protobuf library should end with '_proto' suffix")
  endif()

  # Check that linter config file is present.
  if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/buf.yaml)
    message(FATAL_ERROR "Protobuf library directory must contain buf.yaml file")
  endif()

  find_package(Protobuf REQUIRED)

  set(PROTOBUF_GENERATE_CPP_APPEND_PATH FALSE)

  # Collect paths containing .proto dependencies.
  set(PROTOBUF_DEP_PATHS "")
  foreach(DEP_LIB ${TARGET_ARG_PUBLIC_LINK_LIBS})
    get_target_property(DEP_PATH ${DEP_LIB} PROTOBUF_IMPORT_PATH)
    list(APPEND PROTOBUF_DEP_PATHS ${DEP_PATH})
  endforeach()

  list(REMOVE_DUPLICATES PROTOBUF_DEP_PATHS)

  # This var should contain paths to all .proto files in use, even imported.
  set(PROTOBUF_IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${PROTOBUF_DEP_PATHS})

  protobuf_generate_cpp(PROTOBUF_SOURCES PROTOBUF_HEADERS ${TARGET_ARG_SOURCES})

  set(PROTOBUF_LIBRARY eolo_${TARGET_ARG_NAME})

  message(STATUS "Create protobuf library: ${PROTOBUF_LIBRARY}")

  add_library(${PROTOBUF_LIBRARY} ${PROTOBUF_SOURCES} ${PROTOBUF_HEADERS})
  set(LIBRARY_NAME_ALIAS ${CMAKE_PROJECT_NAME}::${TARGET_ARG_NAME})
  add_library(${LIBRARY_NAME_ALIAS} ALIAS ${PROTOBUF_LIBRARY})

  # NOTE A proto-library should provide a path to its root dir, so dependent libs could find its .proto files. Also, the
  # paths to the dependencies of dependencies should be present.
  set_target_properties(${PROTOBUF_LIBRARY} PROPERTIES PROTOBUF_IMPORT_PATH "${PROTOBUF_IMPORT_DIRS}")

  target_link_libraries(
    ${PROTOBUF_LIBRARY}
    PUBLIC ${TARGET_ARG_PUBLIC_LINK_LIBS} protobuf::libprotobuf
    PRIVATE ${TARGET_ARG_PRIVATE_LINK_LIBS}
  )

  target_include_directories(
    ${PROTOBUF_LIBRARY} SYSTEM # for clang-tidy to ignore header files from this directory
    BEFORE PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> $<INSTALL_INTERFACE:install>
  )

  # Register protobuf library as a dependency for the module.
  set(MODULE_${MODULE_NAME}_LIB_TARGETS
      ${MODULE_${MODULE_NAME}_LIB_TARGETS} ${PROTOBUF_LIBRARY}
      CACHE INTERNAL "Targets in module ${MODULE_NAME}"
  )

  set_target_properties(
    ${PROTOBUF_LIBRARY}
    PROPERTIES VERSION ${VERSION}
               SOVERSION ${VERSION_MAJOR}
               EXPORT_NAME ${PROTOBUF_LIBRARY}
  )

  # Disable clang-tidy for the generated source and header files.
  set_target_properties(${PROTOBUF_LIBRARY} PROPERTIES CXX_CLANG_TIDY "")
endmacro()

# ==================================================================================================
# Setup tests target

FetchContent_Declare(
  googletest URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE
)
FetchContent_MakeAvailable(googletest)
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# use leaner set of compiler warnings for sources we have no control over
set_target_properties(gtest PROPERTIES COMPILE_OPTIONS "${THIRD_PARTY_COMPILER_WARNINGS}")
set_target_properties(gtest PROPERTIES CXX_CLANG_TIDY "")
set_target_properties(gtest_main PROPERTIES CXX_CLANG_TIDY "")
set_target_properties(gmock PROPERTIES COMPILE_OPTIONS "${THIRD_PARTY_COMPILER_WARNINGS}")
set_target_properties(gmock PROPERTIES CXX_CLANG_TIDY "")
set_target_properties(gmock_main PROPERTIES CXX_CLANG_TIDY "")

# ==================================================================================================
# Adds a custom target to group all test programs built on call to `make tests`
add_custom_target(tests COMMENT "Building tests")

# ==================================================================================================
# Adds a custom convenience target to run all tests and generate report on 'make check'
add_custom_target(
  check
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --output-log tests_log.txt --verbose
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Building and running tests"
)
add_dependencies(check tests) # `check` depends on `tests` target

# ==================================================================================================
# macro: define_module_test
#
# Description: Macro to define a 'test' program Note: - A test program belongs to the enclosing module - A module can
# have multiple tests - Executables are not installed on call to `make install`
#
# Parameters: SOURCES     : (list) Source files to compile [PUBLIC_INCLUDE_PATHS] : (list, optional) Publicly included
# directories. See cmake documentation for 'PUBLIC' keyword in `target_include_directories` [PRIVATE_INCLUDE_PATHS] :
# (list, optional) Privately included directories. See cmake documentation for 'PRIVATE' keyword in
# `target_include_directories` [SYSTEM_PUBLIC_INCLUDE_PATHS] : (list, optional) Publicly included system directories on
# some platforms. See 'SYSTEM' keyword in `target_include_directories` [SYSTEM_PRIVATE_INCLUDE_PATHS] : (list, optional)
# Privately included system directories on some platforms. See 'SYSTEM' keyword in `target_include_directories`
# [PUBLIC_LINK_LIBS] : (list, optional) Public link dependencies. See 'PUBLIC' keyword in `target_link_libraries`
# [PRIVATE_LINK_LIBS]: (list, optional) Private link dependencies. See 'PRIVATE' keyword in `target_link_libraries`
#
include(GoogleTest)
macro(define_module_test)
  set(flags "")
  set(single_opts NAME COMMAND WORKING_DIRECTORY)
  set(multi_opts SOURCES PUBLIC_INCLUDE_PATHS PRIVATE_INCLUDE_PATHS PUBLIC_LINK_LIBS PRIVATE_LINK_LIBS)

  include(CMakeParseArguments)
  cmake_parse_arguments(TARGET_ARG "${flags}" "${single_opts}" "${multi_opts}" ${ARGN})

  if(TARGET_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments: ${TARGET_ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT TARGET_ARG_NAME)
    message(FATAL_ERROR "Executable name not specified")
  endif()

  set(TARGET_NAME ${CMAKE_PROJECT_NAME}_${MODULE_NAME}_${TARGET_ARG_NAME})

  add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL ${TARGET_ARG_SOURCES}) # Don't build on `make`
  add_clang_format(${TARGET_NAME})

  # Add to cmake tests (call using ctest)
  add_test(
    NAME ${TARGET_NAME}
    COMMAND ${TARGET_NAME}
    WORKING_DIRECTORY ${TARGET_ARG_WORKING_DIRECTORY}
  )
  add_dependencies(tests ${TARGET_NAME}) # Set this to be built on `make tests`

  target_include_directories(
    ${TARGET_NAME} BEFORE
    PUBLIC ${TARGET_ARG_PUBLIC_INCLUDE_PATHS}
    PRIVATE ${TARGET_ARG_PRIVATE_INCLUDE_PATHS}
  )

  target_link_libraries(
    ${TARGET_NAME}
    PUBLIC ${TARGET_ARG_PUBLIC_LINK_LIBS}
    PRIVATE ${MODULE_${MODULE_NAME}_LIB_TARGETS} # link to libraries from the enclosing module
            ${TARGET_ARG_PRIVATE_LINK_LIBS} GTest::gtest GTest::gmock GTest::gtest_main GTest::gmock_main
  )

  gtest_discover_tests(
    ${TARGET_NAME}
    WORKING_DIRECTORY ${TARGET_ARG_WORKING_DIRECTORY}
    PROPERTIES
    TIMEOUT ${TEST_TIMEOUT_SECONDS}
  )

endmacro()

# ==================================================================================================
# (for internal use) Create installation rules for library and executable targets in enabled modules Description:
# Installs CMake config files to support calling find_package() in downstream projects. The following files are
# configured and installed in <installdir>/lib/cmake/<project>_<module>: - <project>_<module>-config.cmake -
# <project>_<module>-config-version.cmake - <project>_<module>-targets.cmake
function(install_modules)
  get_property(_enabled_modules_list GLOBAL PROPERTY ENABLED_MODULES)
  foreach(_module IN LISTS _enabled_modules_list)

    # These variables are copied into module-config.cmake.in to set up dependencies
    set(INSTALL_MODULE_NAME ${CMAKE_PROJECT_NAME}_${_module})
    set(INSTALL_MODULE_LIB_TARGETS ${MODULE_${_module}_LIB_TARGETS})
    set(INSTALL_MODULE_INTERNAL_DEPENDENCIES ${MODULE_${_module}_DEPENDS_ON})
    set(INSTALL_MODULE_EXTERNAL_DEPENDENCIES ${MODULE_${_module}_EXTERNAL_PROJECT_DEPS})

    set(config_create_location ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${INSTALL_MODULE_NAME})
    set(config_install_location ${CMAKE_INSTALL_LIBDIR}/cmake/${INSTALL_MODULE_NAME})

    # create package config information for third parties
    configure_package_config_file(
      ${CMAKE_TEMPLATES_DIR}/module-config.cmake.in ${config_create_location}/${INSTALL_MODULE_NAME}-config.cmake
      INSTALL_DESTINATION ${config_install_location}
      PATH_VARS CMAKE_INSTALL_FULL_INCLUDEDIR CMAKE_INSTALL_FULL_LIBDIR
      NO_SET_AND_CHECK_MACRO NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    # create package version information
    write_basic_package_version_file(
      ${config_create_location}/${INSTALL_MODULE_NAME}-config-version.cmake
      VERSION ${VERSION}
      COMPATIBILITY AnyNewerVersion
    )

    # install package targets
    message(VERBOSE "Module \"${_module}\" installable targets:")
    message(VERBOSE "  Library targets\t : ${MODULE_${_module}_LIB_TARGETS}")
    message(VERBOSE "  Executable targets\t : ${MODULE_${_module}_EXE_TARGETS}")
    install(
      TARGETS ${MODULE_${_module}_LIB_TARGETS} ${MODULE_${_module}_EXE_TARGETS}
      EXPORT ${INSTALL_MODULE_NAME}-targets
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      INCLUDES
      DESTINATION include
    )

    # install public header files
    if(EXISTS ${MODULE_${_module}_PATH}/include)
      install(DIRECTORY ${MODULE_${_module}_PATH}/include/ DESTINATION include)
    endif()

    # export targets
    install(
      EXPORT ${INSTALL_MODULE_NAME}-targets
      FILE ${INSTALL_MODULE_NAME}-targets.cmake
      NAMESPACE ${CMAKE_PROJECT_NAME}::
      DESTINATION ${config_install_location}
    )

    # install config files
    install(FILES "${config_create_location}/${INSTALL_MODULE_NAME}-config.cmake"
                  "${config_create_location}/${INSTALL_MODULE_NAME}-config-version.cmake"
            DESTINATION ${config_install_location}
    )
  endforeach()

  # configure the uninstaller script
  configure_file(${CMAKE_TEMPLATES_DIR}/uninstall.sh.in "${CMAKE_BINARY_DIR}/uninstall.sh" @ONLY)

  # install the uninstaller script
  install(
    FILES "${CMAKE_BINARY_DIR}/uninstall.sh"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    PERMISSIONS
      OWNER_EXECUTE
      OWNER_READ
      OWNER_WRITE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE
  )

  # To support 'uninstall', process and append build manifest and set it to be installed too
  install(
    CODE "string(REPLACE \";\" \"\\n\" MY_CMAKE_INSTALL_MANIFEST_CONTENT \"\$\{CMAKE_INSTALL_MANIFEST_FILES\}\")\n\
  file(WRITE ${CMAKE_BINARY_DIR}/manifest.txt \"\$\{MY_CMAKE_INSTALL_MANIFEST_CONTENT\}\")"
  )
  install(FILES "${CMAKE_BINARY_DIR}/manifest.txt" DESTINATION share/${CMAKE_PROJECT_NAME})

endfunction()

# ==================================================================================================
# (for internal use) Recursively walk through dependencies of the given module and mark them for building. Set
# ENABLED_MODULES property to a list of modules that should be built. The list is sorted by modules dependencies, i.e.
# each module is preceded by its dependencies. Set EXTERNAL_PROJECTS property to a list of external projects required by
# the enabled modules.
function(mark_module_and_dependencies_to_build)
  set(flags "")
  set(single_opts MODULE_NAME)
  set(multi_opts "")
  include(CMakeParseArguments)

  cmake_parse_arguments(_ARG "${flags}" "${single_opts}" "${multi_opts}" ${ARGN})

  if(_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments: ${_ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT _ARG_MODULE_NAME)
    message(FATAL_ERROR "MODULE_NAME not specified.")
  endif()

  # Check if the module has been processed already (e.g. as a dependency of other module).
  get_property(WAS_PROCESSED GLOBAL PROPERTY MODULE_${_ARG_MODULE_NAME}_PROCESSED)
  if(WAS_PROCESSED)
    return()
  endif()

  define_property(
    GLOBAL
    PROPERTY MODULE_${_ARG_MODULE_NAME}_ALL_DEPS
    BRIEF_DOCS
      "A list of all module's dependencies sorted by depends-on attribute: the lower level dependencies go first"
    FULL_DOCS
      "A list of all module's dependencies sorted by depends-on attribute: the lower level dependencies go first"
  )

  # Recursively process all dependencies.
  foreach(_dep IN LISTS MODULE_${_ARG_MODULE_NAME}_DEPENDS_ON)
    if(NOT EXISTS ${MODULE_${_dep}_PATH})
      message(FATAL_ERROR "Module \"${_module}\" internal dependency \"${_dep}\" does not exist")
    endif()
    mark_module_and_dependencies_to_build(MODULE_NAME ${_dep})

    # Set a corresponding global property to list of all modules that the module depends on, directly or indirectly.
    get_property(MODULE_DEP_LIST GLOBAL PROPERTY MODULE_${_dep}_ALL_DEPS)
    set_property(GLOBAL APPEND PROPERTY MODULE_${_ARG_MODULE_NAME}_ALL_DEPS ${MODULE_DEP_LIST})
    set_property(GLOBAL APPEND PROPERTY MODULE_${_ARG_MODULE_NAME}_ALL_DEPS ${_dep})
  endforeach()

  # Populate the global list of external dependencies.
  get_property(_external_projects GLOBAL PROPERTY EXTERNAL_PROJECTS)
  list(APPEND _external_projects ${MODULE_${_ARG_MODULE_NAME}_EXTERNAL_PROJECT_DEPS})
  list(REMOVE_DUPLICATES _external_projects)
  set_property(GLOBAL PROPERTY EXTERNAL_PROJECTS ${_external_projects})

  # Construct a final deduplicated dependency list of the module.
  get_property(MODULE_DEP_LIST GLOBAL PROPERTY MODULE_${_ARG_MODULE_NAME}_ALL_DEPS)
  list(REMOVE_DUPLICATES MODULE_DEP_LIST)
  set_property(GLOBAL PROPERTY MODULE_${_ARG_MODULE_NAME}_ALL_DEPS ${MODULE_DEP_LIST})

  message(VERBOSE "Dependencies ${_ARG_MODULE_NAME}: ${MODULE_DEP_LIST}")

  # Populate the global list of enabled modules.
  get_property(_enabled_modules GLOBAL PROPERTY ENABLED_MODULES)
  list(APPEND _enabled_modules ${MODULE_DEP_LIST})
  list(APPEND _enabled_modules ${_ARG_MODULE_NAME})
  list(REMOVE_DUPLICATES _enabled_modules)
  set_property(GLOBAL PROPERTY ENABLED_MODULES ${_enabled_modules})

  set_property(GLOBAL PROPERTY MODULE_${_ARG_MODULE_NAME}_PROCESSED 1)
endfunction()

# ==================================================================================================
# (for internal use) Walk through all enabled modules and their dependencies and mark them for building
function(mark_modules_to_build)
  get_property(_declared_modules GLOBAL PROPERTY DECLARED_MODULES)
  foreach(_module IN LISTS _declared_modules)
    if(${BUILD_MODULE_${_module}})
      mark_module_and_dependencies_to_build(MODULE_NAME ${_module})
    endif()
  endforeach()
endfunction()

# ==================================================================================================
# (for internal use) Recurse through directory tree and find all CMakeLists.txt files that contains module declaration
function(find_module_declarations _result)
  set(flags "")
  set(single_opts ROOT_PATH)
  set(multi_opts "")

  # This is the regex string we search for in the files to find a module declaration
  set(_declaration_search_regex "^declare_module\\(")

  include(CMakeParseArguments)
  cmake_parse_arguments(_ARG "${flags}" "${single_opts}" "${multi_opts}" ${ARGN})

  if(_ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments: ${_ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT _ARG_ROOT_PATH)
    message(FATAL_ERROR "ROOT_PATH not specified.")
  endif()

  # * Find all cmakelists.txt recursively starting from specified path
  # * From these, extract those that contain 'declare_module' string
  file(GLOB_RECURSE _all_cmakelists "${_ARG_ROOT_PATH}/CMakeLists.txt")

  set(_module_files_list)
  foreach(_file ${_all_cmakelists})
    file(STRINGS "${_file}" _lines REGEX ${_declaration_search_regex})
    if(_lines)
      list(APPEND _module_files_list "${_file}")
    endif()
  endforeach()
  set(${_result}
      ${_module_files_list}
      PARENT_SCOPE
  )

endfunction()

# ==================================================================================================
# Documentation target (`make docs`)
#
# The following variables must be set for the documentation generation system to work DOC_INPUT_PATHS (set elsewhere)
# DOC_EXAMPLE_PATHS (set elsewhere) DOC_OUTPUT_PATH (set here)
#
if(NOT CMAKE_CROSSCOMPILING)
  find_package(Doxygen OPTIONAL_COMPONENTS doxygen dot mscgen dia)
  if(DOXYGEN_FOUND)
    set(DOC_OUTPUT_PATH ${CMAKE_BINARY_DIR}/docs/)
    file(MAKE_DIRECTORY ${DOC_OUTPUT_PATH})

    add_custom_target(
      docs
      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/doxyfile
      WORKING_DIRECTORY ${DOC_OUTPUT_PATH}
      SOURCES ""
      COMMENT "Generating API documentation"
      VERBATIM
    )

    # install(DIRECTORY ${DOC_OUTPUT_PATH} DESTINATION ${CMAKE_INSTALL_DOCDIR})
    message(STATUS "Documentation ('make docs') path: ${DOC_OUTPUT_PATH}")
  else()
    message(STATUS "Documentation ('make docs'): Not Available")
  endif(DOXYGEN_FOUND)
endif(NOT CMAKE_CROSSCOMPILING)
