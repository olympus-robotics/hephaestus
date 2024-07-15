# Hephaestus

Hephaestus is a C++ framework designed to facilitate robotics development by providing commonly needed functionality and abstractions.

The goal of Hephaestus is to support the deployment of robotic code in production. This means:
* Simple, stable, performant functionalities.
* No focus on simplifying experimentation and no-nonsense functionality.
    * New features will be added if they make production code better (faster/stable/simpler), not for making running experiments faster (I am looking at you ROS).
* Abstract from the developer as much of the complexity of writing C++ code as possible:
    * Memory managment.
    * Parallelism and multi/threading.
    * Containers and basic algorithm.

Hephaestus provides functionalities that cover the following domains:
* Inter Process Comunication (IPC).
* Data serialization for IPC and storage.
* Multi-threading, e.g. thread pools and parallelism primitive.
* Containers, e.g. thread safe containers for sharing data across threads.
* Memory pool.
* Functionalities to run real-time code.

> NOTE: most of the above functionalities are still work in progress.

When should you use this over ROS? Click [here](comparison_to_ros.md)!

## Build

### Env
The best way to build hephaestus is to do it inside the docker container provided in the `docker` folder. You can build the container with `build.sh` and start it using `run.sh`.

### Compilation

Hephaestus uses CMake to build, the build infrastructure is copied and adapted from [grape](https://github.com/cvilas/grape).

To build it:
```bash
cd hephaestus
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
You can use the build system of `hephaestus` in your own project by importing the CMake files and recreating the required folder structure.

Create the top level `CMakeLists.txt` as:
```cmake
cmake_minimum_required(VERSION 3.22.1)
project(my_project LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    hephaestus
    GIT_REPOSITORY "https://github.com/olympus-robotics/hephaestus.git"
    GIT_TAG "main"
)
FetchContent_GetProperties(hephaestus)
if(NOT hephaestus_POPULATED)
  FetchContent_Populate(hephaestus)
endif()

# If you want to use hephaestus toolchain add:
# set(CMAKE_TOOLCHAIN_FILE ${hephaestus_SOURCE_DIR}/toolchains/toolchain_clang.cmake)

# Include the Cmake functions.
include(${hephaestus_SOURCE_DIR}/cmake/build.cmake)
```

Create the `modules` folder and add your modules. You can use the hephaestus script by calling:
```bash
cd modules
python3 ../build/_deps/hephaestus-src/cmake/create_module.py my_module
```

Create the `external` folder and add a `CMakeLists.txt` file as:
```cmake
cmake_minimum_required(VERSION 3.27.3)
project(my_project-external LANGUAGES C CXX)

include(${CMAKE_TEMPLATE_DIR}/external.cmake)

# Add your desired dependencies:
# add_cmake_dependency(
#     NAME hephaestus
#     GIT_REPOSITORY "https://github.com/olympus-robotics/hephaestus.git"
#     GIT_TAG "main"
#     CMAKE_ARGS -DBUILD_MODULES="utils"
# )
```

## Using Hephaestus
There are multiple ways to use Hephaestus in your repo.

### Global installation
Install hephaestus in a known folder, e.g. `/install`. When you compile your project pass `-DCMAKE_PREFIX_PATH=/install` and in your CMakeLists.txt:

```cmake
find_package(hephaestus REQUIRED <component1> <component2>) # e.g. find_package(hephaestus REQUIRED ipc serdes)

add_library(my-lib ...)
target_link_libraries(my-lib
    PUBLIC hephaestus::ipc hephaestus::serdes
)
```

### Use Hephaestus CMake build system
Using Hephaestus build system build hephaestus together with your project:

```cmake
add_cmake_dependency(
    NAME hephaestus
    GIT_REPOSITORY "https://github.com/olympus-robotics/hephaestus.git"
    GIT_TAG "main"
    CMAKE_ARGS -DBUILD_MODULES="utils"
)
```

and in your library CMakeLists.txt:
```cmake
find_package(hephaestus REQUIRED <component1> <component2>) # e.g. find_package(hephaestus REQUIRED ipc serdes)

add_library(my-lib ...)
target_link_libraries(my-lib
    PUBLIC hephaestus::ipc hephaestus::serdes
)
```

### Include Hephaestus as a submodule
Add Hephaestus as a git submodule to your project (e.g. in `third_party/hephaestus`) and in the root CMakeLists.txt before adding your libraries add:

```cmake
set(BUILD_MODULES "ipc;serdes") # or `all` if you want to build all of it.
add_subdirectory(third_party/hephaestus)
```

if you are using Hephaestus build system for your project you need to backup the modules list to keep it separate between the Hephaestus ones and yours:


```cmake
set(hephaestus_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/hephaestus)

set(BUILD_MODULES_BAK ${BUILD_MODULES})
set(BUILD_MODULES "utils")
add_subdirectory(${hephaestus_SOURCE_DIR})

set(BUILD_MODULES ${BUILD_MODULES_BAK})
include(${hephaestus_SOURCE_DIR}/cmake/build.cmake)
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
