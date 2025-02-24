# README: websocket_bridge

## Overview

This module offers a websocket interface to hephaestus, enabling foxglove studio or a similar websocket-capable client to connect to hephaestus nodes and:
  - Publish and subscribe to hephaestus topics
  - Trigger hephaestus service calls
  - Monitor the hephaestus node graph (Foxglove has a built in node graph visualizer)

## Build

To build the WebSocket bridge, execute the following command:

```bash
bazel build //modules/websocket_bridge:app
```

## Running the WebSocket Bridge

To run the WebSocket bridge, execute the following command:

```bash
bazel run //modules/websocket_bridge::app -- -c <path to config file>
```

## Detailed description



## TODOs

 - [ ] Fix deadlock in IPCGraph when shutting down
 - [ ] Add support for client-side service calls
 - [ ] Add support for client-side topic publishing
 - [ ] Test advanced websocket interface functions:
    - [ ] TLS
    - [ ] Whitelisting/blacklisting
    - [ ] Compression
