# eolo

## Build

### Env
The best way to build eolo is to do it inside the docker container provided in the `docker` folder. You can build the container with `build.sh` and start it using `run.sh`.

### Compilation

Eolo uses CMake to build, the build infrastructure is heavily inspired (see copied) from [grape](https://github.com/cvilas/grape).

## Notes

Initially this repo was supporting C++23, but to maximise compatibilty I reverted back to C++20.

When switching again back to C++23 it will be possible to remove `fmt` and `ranges-v3`. The transition will be easy, just rename `fmt::` -> `std::` and remove `fmt::formatter`.

## TODO

`devenv`
- [] Add a new docker image on top of the existing one that build the dependencies
  - Understand how we can automatically re-build if something changes

`zenoh`
- [x] Add liveliness client (i.e. topic list)
- [x] Add scout client (i.e. node list)
- [x] Add publisher subscriber listenr, waiting for https://github.com/eclipse-zenoh/zenoh-c/pull/236
- [x] Add router client
- [] Add option for high performant publisher (disable cache, liveliness, metadata and set high priority)
- [] Bug: ID to string conversion doesn't work for custom specified ID
- [] Add option to specify custom ID for session

`cli`
- [] Add support to `enum` options
- [] Add support to `vector` options

### Serialization
- [] Introduce MCAP
