#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

declare_module(NAME ws_bridge DEPENDS_ON_MODULES "" DEPENDS_ON_EXTERNAL_PROJECTS "")

# library sources
set(SOURCES
        src/ws_bridge.cpp
        README.md
        include/hephaestus/ws_bridge/ws_bridge.h)

# library target
define_module_library(
        NAME ws_bridge
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
add_subdirectory(apps)
