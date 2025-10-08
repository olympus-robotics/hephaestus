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

**************
Project Status
**************

.. note::

   Many features are currently under development. Please check the issue tracker or project boards for current status.

When should you use this over ROS? Click [here](doc/comparison_to_ros.md)!

Scope Limitations
=================

Hephaestus intentionally excludes the following to maintain focus and allow for use-case optimization:

- Data type definitions (images, point clouds, etc.)
- Visualization tools
- Geometric operations
- Autonomy algorithms
- Use-case specific implementations

This deliberate limitation allows teams to implement these features optimally for their specific robotics applications while leveraging Hephaestus's robust infrastructure.

Dev Env
=======

The best way to build Hephaestus is to do it inside the docker container provided in the ``docker`` folder. You can build and start the container with ``make docker-up``.

If you use VS Code, run ``make configure-attach-container`` and then Command Palette (Ctrl+Shift+P) ``Dev Containers: Attach to Running Container...`` -> ``/hephaestus-dev``

Bazel
=====

Bazel is the official tool to build Hephaestus.

Commands
--------

.. code-block:: bash

  # Build:
  bazel build //modules/...

  # You can also build a specific module or library by providing the path: 
  ``bazel build //modules/ipc/...``
  ``bazel build //modules/ipc:zenoh_topic_list``

  # Build with clang-tidy:
  bazel build //modules/... --config clang-tidy``

  # Run tests
  bazel test //modules/...
  
  # Format the code:
  bazel run :format       # fixes the formatting errors
  bazel run :format.check # fails on error

  # Generate ``compile_commands.json``, for VS Code ``clangd`` tool:
  bazel run :refresh_compile_commands

  # Run binaries:
  bazel run //modules/ipc:zenoh_topic_list
  # or:
  ./bazel-bin/modules/ipc/zenoh_topic_list

Clang-tidy Profiling
^^^^^^^^^^^^^^^^^^^^

It is possible to run clang-tidy with profiling enabled to see how long each check takes. To do that:
* in ``clang_tidy.bzl`` uncomment line ``# args.add("--enable-check-profile")``
* in ``run_clang_tidy.sh`` comment current ``trap`` command and uncomment ``# trap 'head -n 15 "$logfile" 1>&2' EXIT``

This will print stats for each files sorted by time.

Folders
^^^^^^^

For more details see https://bazel.build/remote/output-directories

Bazel generates three folders in the workspace:

- ``bazel-bin``: Contains the binaries and release artifacts like debians and packages
- ``bazel-out``: Contains build artifacts, build and test logs
- ``bazel-hephaestus``: Can be ignored

CMake
=====

The following sections contains all the information needed to use Hephaestus with CMake

Compilation
-----------

Hephaestus uses CMake to build, the build infrastructure is copied and adapted from [grape](https://github.com/cvilas/grape).

To build it:

.. code-block:: bash

  cd hephaestus
  mkdir build && cd build
  cmake --preset default ..
  ninja all examples

To compile and run the unit test

.. code-block:: bash

  ninja check

> TODO: add section on the different flags and options.

Build system
^^^^^^^^^^^^

You can use the build system of ``hephaestus`` in your own project by importing the CMake files and recreating the required folder structure.

Create the top level ``CMakeLists.txt`` as:

.. code-block:: cmake

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
  # set(CMAKE_TOOLCHAIN_FILE ${hephaestus_SOURCE_DIR}/cmake/toolchains/toolchain_clang.cmake)

  # Include the Cmake functions.
  include(${hephaestus_SOURCE_DIR}/cmake/build.cmake)

Create the ``modules`` folder and add your modules. You can use the hephaestus script by calling:
.. code-block:: bash

  cd modules
  python3 ../cmake/create_module.py my_module

Create the ``external`` folder and add a ``CMakeLists.txt`` file as:

.. code-block:: cmake

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

Using Hephaestus
----------------

There are multiple ways to use Hephaestus in your repo.

Global installation
^^^^^^^^^^^^^^^^^^^

Install hephaestus in a known folder, e.g. ``/install``. When you compile your project pass ``-DCMAKE_PREFIX_PATH=/install`` and in your CMakeLists.txt:

.. code-block:: cmake

  find_package(hephaestus REQUIRED <component1> <component2>) # e.g. find_package(hephaestus REQUIRED ipc serdes)

  add_library(my-lib ...)
  target_link_libraries(my-lib
      PUBLIC hephaestus::ipc hephaestus::serdes
  )

Use Hephaestus CMake build system
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Using Hephaestus build system build hephaestus together with your project:

.. code-block:: cmake

  add_cmake_dependency(
      NAME hephaestus
      GIT_REPOSITORY "https://github.com/olympus-robotics/hephaestus.git"
      GIT_TAG "main"
      CMAKE_ARGS -DBUILD_MODULES="utils"
  )

and in your library CMakeLists.txt:

.. code-block:: cmake

  find_package(hephaestus REQUIRED <component1> <component2>) # e.g. find_package(hephaestus REQUIRED ipc serdes)

  add_library(my-lib ...)
  target_link_libraries(my-lib
      PUBLIC hephaestus::ipc hephaestus::serdes
  )

Include Hephaestus as a submodule
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Add Hephaestus as a git submodule to your project (e.g. in ``third_party/hephaestus``) and in the root CMakeLists.txt before adding your libraries add:

.. code-block:: cmake

  set(BUILD_MODULES "ipc;serdes") # or ``all`` if you want to build all of it.
  add_subdirectory(third_party/hephaestus)

if you are using Hephaestus build system for your project you need to backup the modules list to keep it separate between the Hephaestus ones and yours:


.. code-block:: cmake

  set(hephaestus_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/hephaestus)

  set(BUILD_MODULES_BAK ${BUILD_MODULES})
  set(BUILD_MODULES "utils")
  add_subdirectory(${hephaestus_SOURCE_DIR})

  set(BUILD_MODULES ${BUILD_MODULES_BAK})
  include(${hephaestus_SOURCE_DIR}/cmake/build.cmake)

Linting
=======
We've added shell script linting using shellcheck. To do use this locally, add `.github/scripts/shellcheck.sh` to your pre-commit file.

Notes
=====

Initially this repo was supporting C++23, but to maximize compatibilty we reverted back to C++20.

When switching again back to C++23 it will be possible to remove ``fmt`` and ``ranges-v3``. The transition will be easy, just rename ``fmt::`` -> ``std::`` and remove ``fmt::formatter``.

TODO
====
Hephaestus is under active development. The list of tasks to be carried out can be found under the `Hephaestus Masterplan <https://github.com/orgs/olympus-robotics/projects/2>`__ project.

