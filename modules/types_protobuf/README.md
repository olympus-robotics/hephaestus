# README: types_protobuf

## Brief

Module **types_protobuf** provides mapping of **types** data structures to proto IDL.

## Detailed description

Protobuf is used to serialize all data. Protobuf files are human readable, and export to JSON is supported out of the box. This allows to dump data into files and read it back later in a human readable format.

### Goals
* proto IDL files for a type are feature complete, i.e. serializing and deserializing leads to the exact same data structure
* Types are independent of their serialization method, but types_protobus depends on types.
