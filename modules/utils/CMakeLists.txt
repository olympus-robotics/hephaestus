# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME utils
  DEPENDS_ON_MODULES "" # **Note**: This module should not depend on any other module
  DEPENDS_ON_EXTERNAL_PROJECTS fmt
  ALWAYS_BUILD
)

find_package(fmt REQUIRED)

# generate version header
configure_file(src/version.in ${CMAKE_CURRENT_SOURCE_DIR}/src/version_impl.h @ONLY)

# library sources
set(SOURCES
    src/string/string_utils.cpp
    src/concepts.cpp
    src/exception.cpp
    src/utils.cpp
    src/version.cpp
    README.md
    include/hephaestus/utils/string/string_utils.h
    include/hephaestus/utils/concepts.h
    include/hephaestus/utils/exception.h
    include/hephaestus/utils/utils.h
    include/hephaestus/utils/version.h
)

# library target
define_module_library(
  NAME utils
  PUBLIC_LINK_LIBS fmt::fmt
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(apps)
