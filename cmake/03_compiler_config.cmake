# =================================================================================================
# Copyright (C) 2018 GRAPE Contributors Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # required by source analysis tools

# Build shared libraries by default
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Baseline compiler warning settings for project and external targets
set(HEPHAESTUS_COMPILER_WARNINGS -Wall -Wextra -Wpedantic -Werror)
set(THIRD_PARTY_COMPILER_WARNINGS -Wall -Wextra -Wpedantic)

# clang warnings
set(CLANG_WARNINGS
    -Wshadow # warn the user if a variable declaration shadows one from a parent context
    -Wnon-virtual-dtor # warn if a class with virtual functions has a non-virtual destructor.
    -Wold-style-cast # warn for c-style casts
    -Wcast-align # warn for potential performance problem casts
    -Wunused # warn on anything being unused
    -Woverloaded-virtual # warn if you overload (not override) a virtual function
    -Wpedantic # warn if non-standard C++ is used
    -Wconversion # warn on type conversions that may lose data
    -Wsign-conversion # warn on sign conversions
    -Wnull-dereference # warn if a null dereference is detected
    -Wdouble-promotion # warn if float is implicit promoted to double
    -Wformat=2 # warn on security issues around functions that format output (ie printf)
    -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
)

# GCC warnings
set(GCC_WARNINGS
    ${CLANG_WARNINGS}
    -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
    -Wduplicated-cond # warn if if / else chain has duplicated conditions
    -Wduplicated-branches # warn if if / else branches have duplicated code
    -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
    -Wuseless-cast # warn if you perform a cast to the same type
)

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  add_compile_options(-fcolor-diagnostics)
  set(HEPHAESTUS_COMPILER_WARNINGS ${HEPHAESTUS_COMPILER_WARNINGS} ${CLANG_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  add_compile_options(-fdiagnostics-color=always)
  set(HEPHAESTUS_COMPILER_WARNINGS ${HEPHAESTUS_COMPILER_WARNINGS} ${GCC_WARNINGS})
else()
  message(FATAL_ERROR "Unsupported compiler '${CMAKE_CXX_COMPILER_ID}'")
endif()

# set those compiler warning options
add_compile_options(${HEPHAESTUS_COMPILER_WARNINGS})

# enable sanitizer options
set(SANITIZERS "")

option(ENABLE_ASAN "Enable address sanitizer" FALSE)
if(ENABLE_ASAN)
  list(APPEND SANITIZERS "address")
endif()

option(ENABLE_LSAN "Enable run-time memory leak detector" FALSE)
if(${ENABLE_LSAN})
  list(APPEND SANITIZERS "leak")
endif()

option(ENABLE_UBSAN "Enable undefined behavior sanitizer" FALSE)
if(ENABLE_UBSAN)
  list(APPEND SANITIZERS "undefined")
endif()

option(ENABLE_TSAN "Enable thread sanitizer" FALSE)
if(${ENABLE_TSAN})
  if("address" IN_LIST SANITIZERS OR "leak" IN_LIST SANITIZERS)
    message(FATAL_ERROR "Thread sanitizer does not work with Address and Leak sanitizer enabled")
  else()
    list(APPEND SANITIZERS "thread")
  endif()
endif()

option(ENABLE_MSAN "Enable memory sanitizer" FALSE)
if(${ENABLE_MSAN})
  if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    message(
      WARNING
        "Memory sanitizer requires all the code (including libc++) to be MSAN-instrumented otherwise it reports false positives"
    )
  endif()
  if("address" IN_LIST SANITIZERS
     OR "thread" IN_LIST SANITIZERS
     OR "leak" IN_LIST SANITIZERS
  )
    message(FATAL_ERROR "Memory sanitizer does not work with Address, Thread or Leak sanitizer enabled")
  else()
    list(APPEND SANITIZERS "memory")
  endif()
endif()

list(JOIN SANITIZERS "," LIST_OF_SANITIZERS)
if(LIST_OF_SANITIZERS)
  if(NOT "${LIST_OF_SANITIZERS}" STREQUAL "")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=${LIST_OF_SANITIZERS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${LIST_OF_SANITIZERS}")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=${LIST_OF_SANITIZERS}")
  endif()
endif()

# Set whether to enable inter-procedural optimisation TODO: This is working with latest release of Clang.
option(ENABLE_IPO "Enable interprocedural optimization" OFF)
if(ENABLE_IPO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_result OUTPUT output)
  if(ipo_result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    cmake_policy(SET CMP0069 NEW) # For submodules (eg: googletest) that use CMake older than v3.8
    set(CMAKE_POLICY_DEFAULT_CMP0069 NEW) # About: Run "cmake --help-policy CMP0069"
  else()
    message(SEND_ERROR "Interprocedural optimisation is not supported: ${output}")
  endif()
endif()

# Configure compiler cache
option(ENABLE_CACHE "Enable cache if available" ON)
if(ENABLE_CACHE)
  set(CACHE_PROGRAM_OPTIONS "sccache" "ccache")
  foreach(CACHE_OPTION IN LISTS CACHE_PROGRAM_OPTIONS)
    find_program(CACHE_BIN ${CACHE_OPTION})
    if(CACHE_BIN)
      set(CMAKE_CXX_COMPILER_LAUNCHER ${CACHE_BIN})
      set(CMAKE_C_COMPILER_LAUNCHER ${CACHE_BIN})
      break()
    endif()
  endforeach()
  if(NOT CACHE_BIN)
    message(WARNING "Compiler cache requested but none found")
  endif()
endif()

# Configure test coverage
option(ENABLE_COVERAGE "Collect coverage data" OFF)
if(ENABLE_COVERAGE)
  add_compile_options("--coverage")
  add_link_options("--coverage")
endif()

# Linter (clang-tidy)
option(ENABLE_LINTER "Enable static analysis" ON)
if(ENABLE_LINTER)
  find_program(LINTER_BIN NAMES clang-tidy QUIET)
  if(LINTER_BIN)
    option(ENABLE_LINTER_CACHE "Enable static analysis cache" ON)
    if(ENABLE_LINTER_CACHE)
      # NOTE: To speed up linting, clang-tidy is invoked via clang-tidy-cache.
      # (https://github.com/matus-chochlik/ctcache) Cache location is set by environment variable CTCACHE_DIR
      set(LINTER_INVOKE_COMMAND ${CMAKE_TEMPLATES_DIR}/clang-tidy-cache.py ${LINTER_BIN} -p ${CMAKE_BINARY_DIR}
                                -extra-arg=-Wno-ignored-optimization-argument -extra-arg=-Wno-unknown-warning-option
      )
    else()
      set(LINTER_INVOKE_COMMAND ${LINTER_BIN})
    endif()
    set(CMAKE_C_CLANG_TIDY ${LINTER_INVOKE_COMMAND})
    set(CMAKE_CXX_CLANG_TIDY ${LINTER_INVOKE_COMMAND})
  else()
    message(WARNING "Linter (clang-tidy) not found.")
  endif()
endif()

# print summary
message(STATUS "Compiler configuration:")
message(STATUS "\tCompiler                       : ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "\tBUILD_SHARED_LIBS              : ${BUILD_SHARED_LIBS}")
message(STATUS "\tENABLE_CACHE                   : ${ENABLE_CACHE} (${CACHE_BIN})")
message(STATUS "\tENABLE_IPO                     : ${ENABLE_IPO}")
message(STATUS "\tENABLE_ASAN                    : ${ENABLE_ASAN}")
message(STATUS "\tENABLE_LSAN                    : ${ENABLE_LSAN}")
message(STATUS "\tENABLE_MSAN                    : ${ENABLE_MSAN}")
message(STATUS "\tENABLE_TSAN                    : ${ENABLE_TSAN}")
message(STATUS "\tENABLE_UBSAN                   : ${ENABLE_UBSAN}")
message(STATUS "\tENABLE_COVERAGE                : ${ENABLE_COVERAGE}")
message(STATUS "\tENABLE_LINTER                  : ${ENABLE_LINTER} (${LINTER_BIN})")
message(STATUS "\tEnabled warnings (hephaestus)        : ${HEPHAESTUS_COMPILER_WARNINGS}")
message(STATUS "\tEnabled warnings (third-party) : ${THIRD_PARTY_COMPILER_WARNINGS}")
