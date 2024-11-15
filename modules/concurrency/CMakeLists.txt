# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME concurrency
  DEPENDS_ON_MODULES utils containers random
  DEPENDS_ON_EXTERNAL_PROJECTS absl fmt
)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)

# library sources
set(SOURCES
    src/message_queue_consumer.cpp
    src/spinner.cpp
    src/spinner_manager.cpp
    src/spinner_state_machine.cpp
    README.md
    include/hephaestus/concurrency/message_queue_consumer.h
    include/hephaestus/concurrency/spinner.h
    include/hephaestus/concurrency/spinner_manager.h
    include/hephaestus/concurrency/spinner_state_machine.h
)

# library target
define_module_library(
  NAME concurrency
  PUBLIC_LINK_LIBS absl::log hephaestus::containers hephaestus::utils fmt::fmt
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
add_subdirectory(examples)
