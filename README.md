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

## Build system
You can use the build system of `eolo` in your own project by importing the CMake files and recreating the required folder structure.

Create the top level `CMakeLists.txt` as:
```cmake
cmake_minimum_required(VERSION 3.27.3)
project(my_project LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    eolo
    GIT_REPOSITORY "https://github.com/filippobrizzi/eolo.git"
    GIT_TAG "main"
)
FetchContent_GetProperties(eolo)
if(NOT eolo_POPULATED)
  FetchContent_Populate(eolo)
endif()

# If you want to use eolo toolchain add:
# set(CMAKE_TOOLCHAIN_FILE ${eolo_SOURCE_DIR}/toolchains/toolchain_clang.cmake)

# Include the Cmake functions.
include(${eolo_SOURCE_DIR}/cmake/build.cmake)
```

Create the `modules` folder and add your modules. You can use the eolo script by calling:
```bash
cd modules
python3 ../build/_deps/eolo-src/cmake/create_module.py my_module
```

Create the `external` folder and add a `CMakeLists.txt` file as:
```cmake
cmake_minimum_required(VERSION 3.27.3)
project(my_project-external LANGUAGES C CXX)

include(${CMAKE_TEMPLATE_DIR}/external.cmake)

# Add your desired dependencies:
# add_cmake_dependency(
#     NAME eolo
#     GIT_REPOSITORY "https://github.com/filippobrizzi/eolo.git"
#     GIT_TAG "main"
#     CMAKE_ARGS -DBUILD_MODULES="utils"
# )
```

## Using Eolo
There are multiple ways to use Eolo in your repo.

### Global installation
Install eolo in a known folder, e.g. `/install`. When you compile your project pass `-DCMAKE_PREFIX_PATH=/install` and in your CMakeLists.txt:

```cmake
find_package(eolo REQUIRED <component1> <component2>) # e.g. find_package(eolo REQUIRED ipc serdes)

add_library(my-lib ...)
target_link_libraries(my-lib
    PUBLIC eolo::ipc eolo::serdes
)
```

### Use Eolo CMake build system
Using Eolo build system build eolo together with your project:

```cmake
add_cmake_dependency(
    NAME eolo
    GIT_REPOSITORY "https://github.com/filippobrizzi/eolo.git"
    GIT_TAG "main"
    CMAKE_ARGS -DBUILD_MODULES="utils"
)
```

and in your library CMakeLists.txt:
```cmake
find_package(eolo REQUIRED <component1> <component2>) # e.g. find_package(eolo REQUIRED ipc serdes)

add_library(my-lib ...)
target_link_libraries(my-lib
    PUBLIC eolo::ipc eolo::serdes
)
```

### Include Eolo as a submodule
Add Eolo as a git submodule to your project (e.g. in `third_party/eolo`) and in the root CMakeLists.txt before adding your libraries add:

```cmake
set(BUILD_MODULES "ipc;serdes") # or `all` if you want to build all of it.
add_subdirectory(third_party/eolo)
```

if you are using Eolo build system for your project you need to backup the modules list to keep it separate between the Eolo ones and yours:


```cmake
set(eolo_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/eolo)

set(BUILD_MODULES_BAK ${BUILD_MODULES})
set(BUILD_MODULES "utils")
add_subdirectory(${eolo_SOURCE_DIR})

set(BUILD_MODULES ${BUILD_MODULES_BAK})
include(${eolo_SOURCE_DIR}/cmake/build.cmake)
```

## Notes

Initially this repo was supporting C++23, but to maximise compatibilty we reverted back to C++20.

When switching again back to C++23 it will be possible to remove `fmt` and `ranges-v3`. The transition will be easy, just rename `fmt::` -> `std::` and remove `fmt::formatter`.

## TODO

`devenv`
- [x] Add a new docker image on top of the existing one that build the dependencies
  - [x] Understand how we can automatically re-build if something changes
- [x] Improve usage of Github action as CI

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
