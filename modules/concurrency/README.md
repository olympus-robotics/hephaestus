# Concurrency Module

The Concurrency module provides a set of classes and functions to help with multithreaded programming. It includes utilities for managing threads, synchronizing data access, and handling asynchronous tasks.

## Spinner Component

One of the key components of the Concurrency module is the `Spinner` class. The `Spinner` class provides a simple way to run a task in a separate thread and stop it when necessary.

### Usage

To use the `Spinner` class, you need to create a subclass and implement the `spinOnce` method. This method will be called repeatedly in a separate thread when the spinner is started.
Check the [example](examples/spinner_example.cpp) for more details.

### Stopping the Spinner

The `Spinner` class provides a `stop` method to stop the spinner. You can also add a stop callback that will be called when the spinner is stopped.
Check the [test](tests/spinner_tests.cpp) for more details.

### Thread Safety

The `Spinner` class is thread-safe. You can safely call `start`, `stop`, and `addStopCallback` from multiple threads without additional synchronization.
