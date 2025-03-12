# Module - WebSocket Bridge

## Overview

This module offers a websocket interface to hephaestus, enabling foxglove studio or a similar websocket-capable client to connect to hephaestus communication and:

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
bazel run //modules/websocket_bridge:app -- -c <path to config file>
```

## Bridge Structure

```mermaid
classDiagram
class WebsocketBridge {
    - WebsocketBridgeConfig
    - WebsocketBridgeState
    - IpcGraph
    - IpcEntityManager

    - Foxglove WebSocket Server
    
    + start()
    + stop()

    + callback__Ws__*()
    + callback_IpcGraph_*()
    + callback_Ipc_*()
}

class WebsocketBridgeConfig {
    - WS Server Config
    - IPC Config
    - Topic/Service Whitelists/Blacklists
}

class WebsocketBridgeState {
   - Map: IPC Topics <-> WS Channels
   - Map: WS Channels <-> WS Clients
   - Map: IPC Services <-> WS Services
   - Map: WS Services <-> WS Clients

   + add/remove/has...()
}

class IpcGraph {
    - Topic/Type Database
    - Discovery

    + start()
    + stop()

    + endPointInfoUpdateCallback()

    + add/removePublisherEndpoint()
    + add/removeSubscriberEndpoint()
    + topicHasAnyEndpoint()
    + add/remove/hasTopic()

    + add/remove/hasServiceServerEndpoint()
    + add/remove/hasServiceClientEndpoint()
    + add/remove/hasService()
}

class IpcEntityManager {
    - Subscribers
    - Publishers
    - ServiceClients

    + start()
    + stop()

    + add/remove/hasSubscriber()
    + add/remove/hasPublisher()

    + callService()
    + callServiceAsync()

    + callback__ServiceResponse()
}

class Foxglove WebSocket Server {
    - State/Clients
    - Config

    + start()
    + stop()

    + add/removeChannels()    
    + sendMessage()

    + add/removeServices()    
    + sendServiceResponse()
    + sendServiceError()

    + updateConnectionGraph()    
}

WebsocketBridge --> WebsocketBridgeConfig
WebsocketBridge --> WebsocketBridgeState
WebsocketBridge --> IpcGraph
WebsocketBridge --> IpcEntityManager
WebsocketBridge --> Foxglove WebSocket Server
```

## Example Clients

### Example Client - Services - C++

This client tests the websocket bridge by checking the advertised services and then calling the first one many times while tracking the response times.

```bash
bazel run //modules/websocket_bridge:example_client__services ws://localhost:8765
```

### Example Client - Topics - C++

This client tests the websocket bridge by subscribing to all topics and then advertising mirror topics and pinging back the received messages to the bridge.

```bash
bazel run //modules/websocket_bridge:example_client__topics ws://localhost:8765
```

### Foxglove Studio

1. Open [Foxglove Studio](https://foxglove.dev/download)
2. Add a WebSocket connection
3. Connect to ws://localhost:8765

Note: Service Calls and client-side topic publishing are not supported in Foxglove Studio with protobuf yet.
