# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors MIT License
# =================================================================================================

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(FATAL_ERROR "GCC toolchain is not currently implemented for Darwin.")
else()
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_CXX_COMPILER g++)
endif()
set(CMAKE_CXX_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS "")
set(CMAKE_CROSSCOMPILING FALSE)
