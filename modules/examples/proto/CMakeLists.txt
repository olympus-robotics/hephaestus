#=================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
#=================================================================================================

set(SOURCES
    eolo/examples/types/proto/geometry.proto
    eolo/examples/types/proto/pose.proto
)

define_module_proto_library(NAME examples_types_proto SOURCES ${SOURCES})
