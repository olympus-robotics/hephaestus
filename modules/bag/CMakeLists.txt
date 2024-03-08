# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME bag
  DEPENDS_ON_MODULES containers ipc
  DEPENDS_ON_EXTERNAL_PROJECTS absl mcap
)

find_package(absl REQUIRED)
find_package(mcap REQUIRED)

# library sources
set(SOURCES
    src/zenoh_recorder.cpp
    src/topic_filter.cpp
    src/writer.cpp
    README.md
    include/hephaestus/bag/topic_filter.h
    include/hephaestus/bag/zenoh_recorder.h
    include/hephaestus/bag/writer.h
)

# library target
define_module_library(
  NAME bag
  PUBLIC_LINK_LIBS absl::base hephaestus::containers hephaestus::ipc mcap
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
