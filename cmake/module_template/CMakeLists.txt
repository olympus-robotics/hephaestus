#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

declare_module(NAME @module@ DEPENDS_ON_MODULES "" DEPENDS_ON_EXTERNAL_PROJECTS "")

# library sources
set(SOURCES
        src/@module@.cpp
        README.md
        include/hephaestus/@module@/@module@.h)

# library target
define_module_library(
        NAME @module@
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
