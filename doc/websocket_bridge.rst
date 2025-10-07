
################
Websocket Bridge
################

.. mermaid::

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
