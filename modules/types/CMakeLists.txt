# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME types
  DEPENDS_ON_MODULES random
  DEPENDS_ON_EXTERNAL_PROJECTS absl fmt magic_enum range-v3
)

find_package(absl REQUIRED COMPONENTS hash)
find_package(fmt REQUIRED)
find_package(magic_enum REQUIRED)
find_package(range-v3 REQUIRED)

# library sources
set(SOURCES src/bounds.cpp 
            src/dummy_type.cpp 
            src/uuid_v4.cpp 
            README.md 
            include/hephaestus/types/bounds.h
            include/hephaestus/types/dummy_type.h
            include/hephaestus/types/uuid_v4.h
)

# library target
define_module_library(
  NAME types
  PUBLIC_LINK_LIBS absl::hash fmt::fmt hephaestus::random magic_enum::magic_enum range-v3::range-v3
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
