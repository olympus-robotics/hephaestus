#=================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
#=================================================================================================

set(SOURCES
    eolo/types/proto/geometry.proto
    eolo/types/proto/pose.proto
)

define_module_proto_library(NAME types_proto SOURCES ${SOURCES})
