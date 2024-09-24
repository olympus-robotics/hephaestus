# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

set(SOURCES
    hephaestus/types/proto/dummy_type.proto
)

# To create proto messages in another repository that import `hephaestus` proto messages, see readme.
define_module_proto_library(NAME types_gen_proto SOURCES ${SOURCES})
