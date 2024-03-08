# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors MIT License
# =================================================================================================
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(LLVM_PATH /opt/homebrew/Cellar/llvm/17.0.6_1/bin)
  set(CMAKE_C_COMPILER ${LLVM_PATH}/clang)
  set(CMAKE_CXX_COMPILER ${LLVM_PATH}/clang++)
else()
  set(CMAKE_C_COMPILER clang)
  set(CMAKE_CXX_COMPILER clang++)
endif()
set(CMAKE_CXX_FLAGS "-stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "-stdlib=libc++ -lc++abi")
set(CMAKE_CROSSCOMPILING FALSE)
