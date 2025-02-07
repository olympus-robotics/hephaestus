# README: websocket_bridge

## Overview

This module bridges Zenoh-based communication over a WebSocket interface, enabling real-time data exchange between distributed systems and browser-based clients. It supports secure connections and leverages the underlying Zenoh foundation for efficient message routing and management.

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

Provide more details on the functionality, API usage, etc
