.. _hephaestus:

##########
Hephaestus
##########

A high-performance C++ framework for production-ready robotics development.

********
Overview
********

Hephaestus is a C++ framework designed for deploying robotic systems in production environments. It provides essential, performance-critical functionalities while abstracting away common C++ complexities, allowing developers to focus on their robotics-specific implementation.

Unlike other robotics frameworks that prioritize rapid prototyping, Hephaestus emphasizes production-ready code with a focus on performance, stability, and simplicity.

Design Philosophy
=================

- **Production-First**: Features are added only if they improve production code quality through enhanced performance, stability, or simplicity
- **Focused Scope**: Concentrates on common infrastructure needs across robotic platforms while avoiding use-case specific implementations
- **C++ Abstraction**: Reduces cognitive load by handling complex C++ patterns and best practices automatically

Core Features
=============

Current Implementation
----------------------

Hephaestus provides robust abstractions for fundamental robotics infrastructure:

- **Memory Management**
  - Automated memory handling
  - Memory pools for efficient allocation

- **Concurrency**
  - Thread pools
  - Parallelism primitives
  - Thread-safe containers for cross-thread data sharing

- **System Communication**
  - Inter-Process Communication (IPC)
  - Data serialization for communication and storage

- **Performance Optimization**
  - Real-time execution capabilities
  - Telemetry for system monitoring

.. toctree::
   :caption: Modules
   :maxdepth: 1

   bag
   cli
   concurrency
   conduit
   containers
   format
   ipc
   net
   random
   serdes
   telemetry
   types
   utils
   websocket_bridge

.. toctree::
   :caption: Reference
   :maxdepth: 1

   api

*****
Index
*****

* :ref:`genindex`
