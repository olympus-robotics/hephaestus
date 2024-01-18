#=================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
#=================================================================================================

declare_module(NAME serdes DEPENDS_ON_MODULES base DEPENDS_ON_EXTERNAL_PROJECTS "")

# library sources
set(SOURCES
        src/serdes.cpp
        src/protobuf/buffers.cpp
        src/protobuf/protobuf.cpp
        README.md
        include/eolo/serdes/protobuf/concepts.h
        include/eolo/serdes/protobuf/buffers.h
        include/eolo/serdes/protobuf/protobuf.h)

# library target
define_module_library(
        NAME serdes
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
