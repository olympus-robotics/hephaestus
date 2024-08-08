# README: types

## Brief

Module **types** provides some general types.

## Detailed description

Types defined here are rather abstract and general. Project specific types are out of scope of this module.

### Goals
* Define data types which generalize accross projects.
* Aid testing of modules which requre data to test IO.
* Reduce redundancy accross modules.
* `types` module shall serve as an example on how `types` can be integrated when using Hephaestus.

### Non-goals
* Project specific data types are out of scope of `heph/types`. Data is project specific, thus do not impose conventions here for users of Hephaestus. This translates into e.g. not adding types like 6DOF poses to `heph/types`.
* Serialization is out of scope of `types`. It is performed in separate modules.
* File IO is out of scope of `heph/types`. Save to and load from file is performed in separate modules.

### Requirements
* In general, data structure declarations shall only depend on STL data types and containers.
* Data structure declarations shall not depend on large header-only libraries like Eigen to prevent dependency creep and to keep compile times low.
* Add a method `random` which creates random and possibly unrealistic test data. This data is really random, not a realistic simulation.
* Add `operator==` for testing purposes. In general, adding `operator==() = default` is sufficient.
* Unify testing for all data structures where possible.
