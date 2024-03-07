# =================================================================================================
# Copyright (C) 2018 GRAPE Contributors Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

find_package(Git REQUIRED)

# Global configuration for FetchContent used to integrate third-party content directly into codebase
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_QUIET OFF) # some downloads take long. Be verbose so we know status.

# disallow in-source builds
get_filename_component(srcdir "${PROJECT_SOURCE_DIR}" REALPATH)
get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)
if("${srcdir}" STREQUAL "${bindir}")
  message(FATAL_ERROR "In-source build is not supported. Create a separate build directory and try again")
endif()

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Choose build type." FORCE
  )
  # Set the possible values of build type for cmake-gui, ccmake
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  message(STATUS "Build type not specified. Using '${CMAKE_BUILD_TYPE}'")
endif()

# Build-time paths for generated binaries and libs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# The rpath to use for installed targets.
set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib:${CMAKE_INSTALL_PREFIX}/lib")

# Top level config file
configure_package_config_file(
  ${CMAKE_TEMPLATES_DIR}/project-config.cmake.in
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${CMAKE_PROJECT_NAME}/${CMAKE_PROJECT_NAME}-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME}
  PATH_VARS CMAKE_INSTALL_FULL_INCLUDEDIR CMAKE_INSTALL_FULL_LIBDIR
  NO_SET_AND_CHECK_MACRO NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${CMAKE_PROJECT_NAME}-config-version.cmake
  VERSION ${VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${CMAKE_PROJECT_NAME}/${CMAKE_PROJECT_NAME}-config.cmake
              ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/cmake/${CMAKE_PROJECT_NAME}-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME}
)

# print summary
message(STATUS "Build configuration:")
message(STATUS "\tPlatform            : ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "\tBuild type          : ${CMAKE_BUILD_TYPE}")
message(STATUS "\tCMake generator     : ${CMAKE_GENERATOR} (${CMAKE_BUILD_TOOL})")
message(STATUS "\tRuntime output path : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "\tLibrary output path : ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "\tInstallation path   : ${CMAKE_INSTALL_PREFIX}")
