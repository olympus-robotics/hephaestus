#=================================================================================================
# Copyright (C) 2018-2023 EOLO Contributors
#=================================================================================================

declare_module(NAME base DEPENDS_ON_MODULES "" DEPENDS_ON_EXTERNAL_PROJECTS "")

# library sources
set(SOURCES
        src/type_traits.cpp
        README.md
        include/eolo/base/type_traits.h)

# library target
define_module_library(
        NAME base
        PUBLIC_LINK_LIBS ""
        PRIVATE_LINK_LIBS ""
        SOURCES ${SOURCES}
        PUBLIC_INCLUDE_PATHS
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE_INCLUDE_PATHS ""
        SYSTEM_PRIVATE_INCLUDE_PATHS "")

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
