#=================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
#=================================================================================================

declare_module(NAME serdes_generic DEPENDS_ON_MODULES base DEPENDS_ON_EXTERNAL_PROJECTS "")

# library sources
set(SOURCES
        src/concepts.cpp
        src/deserializer.cpp
        src/serializer.cpp
        README.md
        include/eolo/serdes/generic/concepts.h
        include/eolo/serdes/generic/deserializer.h
        include/eolo/serdes/generic/serializer.h)

# library target
define_module_library(
        NAME serdes_generic
        PUBLIC_LINK_LIBS eolo::base
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
