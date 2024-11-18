# =================================================================================================
# Copyright (C) 2023-2024 HEPHAESTUS Contributors
# =================================================================================================

set(SOURCES
    hephaestus/containers/proto/bit_flag.proto
)

# To create proto messages in another repository that import `hephaestus` proto messages, see readme.
define_module_proto_library(NAME containers_gen_proto SOURCES ${SOURCES})
