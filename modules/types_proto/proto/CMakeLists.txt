# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

set(SOURCES
    hephaestus/types/proto/bounds.proto 
    hephaestus/types/proto/dummy_type.proto
    hephaestus/types/proto/numeric_value.proto
)

define_module_proto_library(NAME types_gen_proto SOURCES ${SOURCES})
