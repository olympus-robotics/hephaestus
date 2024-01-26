# eolo

## Build

### Env
The best way to build eolo is to do it inside the docker container provided in the `docker` folder. You can build the container with `build.sh` and start it using `run.sh`.

### Compilation

Eolo uses CMake to build, the build infrastructure is heavily inspired (see copied) from [grape](https://github.com/cvilas/grape).

## TODO

### IPC
`zenoh`
- [x] Add liveliness client (i.e. topic list)
- [x] Add scout client (i.e. node list)
- [x] Add publisher subscriber listenr, waiting for https://github.com/eclipse-zenoh/zenoh-c/pull/236
- [x] Add router client
- [] Add option for high performant publisher (disable cache, liveliness, metadata and set high priority)

`cli`
- [] Add support to `enum` options
- [] Add support to `vector` options

### Serialization
- [] Add new bag module
