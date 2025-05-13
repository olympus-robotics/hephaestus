# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

set(SOURCES
    hephaestus/types/proto/bool.proto
    hephaestus/types/proto/bounds.proto
    hephaestus/types/proto/dummy_type.proto
    hephaestus/types/proto/numeric_value.proto
    hephaestus/types/proto/uuid_v4.proto
)

# To create proto messages in another repository that import `hephaestus` proto messages, see readme.
define_module_proto_library(NAME types_gen_proto SOURCES ${SOURCES})
