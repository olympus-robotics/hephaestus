#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

define_module_executable(
        NAME bag_recorder
        SOURCES bag_recorder.cpp
        PUBLIC_INCLUDE_PATHS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        PUBLIC_LINK_LIBS "")

define_module_executable(
          NAME bag_player
          SOURCES bag_player.cpp
          PUBLIC_INCLUDE_PATHS
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
          PUBLIC_LINK_LIBS "")
