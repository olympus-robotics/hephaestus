#=================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
#=================================================================================================

set(SOURCES
    hephaestus/examples/types/proto/sample.proto
    hephaestus/examples/types/proto/geometry.proto
    hephaestus/examples/types/proto/pose.proto
)

define_module_proto_library(NAME examples_types_gen_proto  SOURCES ${SOURCES})
