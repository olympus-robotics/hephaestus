# README: types_protobuf

## Brief

Module **types_protobuf** provides mapping of **types** data structures to proto IDL.

## Detailed description

Protobuf is used to serialize all data. Protobuf files are human readable, and export to JSON is supported out of the box. This allows to dump data into files and read it back later in a human readable format.

If you create proto messages in another repository that import `hephaestus` proto messages, you need to link against `types_gen_proto`. Example:
```
define_module_proto_library(NAME types_proto SOURCES ${SOURCES} PUBLIC_LINK_LIBS hephaestus::types_gen_proto)
```

### Goals
* proto IDL files for a type are feature complete, i.e. serializing and deserializing leads to the exact same data structure
* provide generic helper functions for serialization (e.g for `std::chrono::time_point`)

### Non-goals
* similar to module **types**, **types_protobuf** is meant to be generic, and project specific (non-generalizable) methods shall not be added

### Requirements
* Types are independent of their serialization method, but the module **types_protobuf** depends on module **types**.
