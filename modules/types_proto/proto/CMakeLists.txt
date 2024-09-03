# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

set(SOURCES
    hephaestus/types/proto/dummy_type.proto
)

define_module_proto_library(NAME types_gen_proto SOURCES ${SOURCES})
