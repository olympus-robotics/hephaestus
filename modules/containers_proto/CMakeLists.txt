#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

declare_module(
        NAME containers_proto
        DEPENDS_ON_MODULES containers serdes
        DEPENDS_ON_EXTERNAL_PROJECTS ""
)

add_subdirectory(proto)

# library sources
set(SOURCES
        README.md
        include/hephaestus/containers_proto/bit_flag.h)

# library target
define_module_library(
        NAME containers_proto
        PUBLIC_LINK_LIBS hephaestus::containers hephaestus::serdes hephaestus::containers_gen_proto
        PRIVATE_LINK_LIBS ""
        SOURCES ${SOURCES}
        PUBLIC_INCLUDE_PATHS
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE_INCLUDE_PATHS ""
        SYSTEM_PRIVATE_INCLUDE_PATHS "")

# Subprojects
add_subdirectory(tests)
