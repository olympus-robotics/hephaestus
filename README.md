# Eolo

Eolo is a C++ framework designed to facilitate robotics development by providing commonly needed functionality and abstractions.

The goal of Eolo is to support the deployment of robotic code in production. This means:
* Simple, stable, performant functionalities.
* No focus on simplifying experimentation and no-nonsense functionality.
    * New features will be added if they make production code better (faster/stable/simpler), not for making running experiments faster (I am looking at you ROS).
* Abstract from the developer as much of the complexity of writing C++ code as possible:
    * Memory managment.
    * Parallelism and multi/threading.
    * Containers and basic algorithm.

Eolo provides functionalities that cover the following domains:
* Inter Process Comunication (IPC).
* Data serialization for IPC and storage.
* Multi-threading, e.g. thread pools and parallelism primitive.
* Containers, e.g. thread safe containers for sharing data across threads.
* Memory pool.
* Functionalities to run real-time code.

> NOTE: most of the above functionalities are still work in progress.

## Build

### Env
The best way to build eolo is to do it inside the docker container provided in the `docker` folder. You can build the container with `build.sh` and start it using `run.sh`.

### Compilation

Eolo uses CMake to build, the build infrastructure is copied and adapted from [grape](https://github.com/cvilas/grape).

To build it:
```bash
cd eolo
mkdir build && cd build
cmake --preset preferred
ninja all examples
```

To compile and run the unit test
```bash
ninja checks
```

> TODO: add section on the different flags and options.


## Notes

Initially this repo was supporting C++23, but to maximise compatibilty we reverted back to C++20.

When switching again back to C++23 it will be possible to remove `fmt` and `ranges-v3`. The transition will be easy, just rename `fmt::` -> `std::` and remove `fmt::formatter`.

## TODO

`devenv`
- [ ] Add a new docker image on top of the existing one that build the dependencies
  - [ ] Understand how we can automatically re-build if something changes
- [ ] Improve usage of Github action as CI

`zenoh`
- [x] Add liveliness client (i.e. topic list)
- [x] Add scout client (i.e. node list)
- [x] Add publisher subscriber listenr, waiting for https://github.com/eclipse-zenoh/zenoh-c/pull/236
- [x] Add router client
- [x] Add option for high performant publisher (disable cache, liveliness, metadata and set high priority)
  - [ ] Make sure added option works
- [ ] Understand how to specify the protocol, an option as been added but doesn't seem to work
- [ ] Bug: ID to string conversion doesn't work for custom specified ID
- [ ] Add option to specify custom ID for session
- [ ] Improve ipc interface to better abstract over zenoh
    - Q: how to abstract tools very specific to zenoh, like node list of topic list

`cli`
- [ ] Add support to `enum` options
- [ ] Add support to `vector` options

### Serialization
- [ ] Introduce MCAP
