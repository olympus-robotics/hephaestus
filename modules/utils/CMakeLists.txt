# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

declare_module(
  NAME utils
  DEPENDS_ON_MODULES "" # **Note**: This module should not depend on any other module
  DEPENDS_ON_EXTERNAL_PROJECTS fmt absl magic_enum range-v3
  ALWAYS_BUILD
)

find_package(absl REQUIRED)
find_package(fmt REQUIRED)
find_package(magic_enum REQUIRED)
find_package(range-v3 REQUIRED)

# library sources
set(SOURCES
    src/filesystem/file.cpp
    src/filesystem/scoped_path.cpp
    src/format/format.cpp
    src/string/string_literal.cpp
    src/string/string_utils.cpp
    src/timing/stop_watch.cpp
    src/timing/watchdog.cpp
    src/concepts.cpp
    src/exception.cpp
    src/utils.cpp
    src/signal_handler.cpp
    src/stack_trace.cpp
    README.md
    include/hephaestus/utils/filesystem/file.h
    include/hephaestus/utils/filesystem/scoped_path.h
    include/hephaestus/utils/format/format.h
    include/hephaestus/utils/string/string_literal.h
    include/hephaestus/utils/string/string_utils.h
    include/hephaestus/utils/timing/stop_watch.h
    include/hephaestus/utils/timing/watchdog.h
    include/hephaestus/utils/concepts.h
    include/hephaestus/utils/exception.h
    include/hephaestus/utils/utils.h
    include/hephaestus/utils/signal_handler.h
    include/hephaestus/utils/stack_trace.h
)

# library target
define_module_library(
  NAME utils
  PUBLIC_LINK_LIBS
    absl::log
    absl::random_random
    absl::time
    fmt::fmt
    magic_enum::magic_enum
    range-v3::range-v3
  PRIVATE_LINK_LIBS ""
  SOURCES ${SOURCES}
  PUBLIC_INCLUDE_PATHS $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>
  PRIVATE_INCLUDE_PATHS ""
  SYSTEM_PRIVATE_INCLUDE_PATHS ""
)

# Subprojects
add_subdirectory(tests)
