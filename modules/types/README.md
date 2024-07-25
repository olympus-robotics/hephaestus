# README: types

## Brief

Module **types** provides some general types.

## Detailed description

Types defined here are rather abstract and general. Project specific types are out of scope of this module.

### Goals
* In general, data structure declarations shall only depend on STL data types and containers.e`)
* Add a method `random` which creates random and possibly unrealistic test data. This is just random data, not a realistic simulation.
* Add `operator==` for testing purposes. In general, adding `operator==() = default` is sufficient.
* Unify testing for all data structures where possible.

### Non-goals
* Data is project specific, thus we do not impose conventions here for users of Hephaestus. This translates into e.g. not adding types like 6DOF poses to heph/types.
* Data structure declarations shall not depend on large header-only libraries like Eigen to prevent dependency creep and to keep compile times low.
* Do not implement Serialization, which is done in a separate module.
* Do not implement load from file, which is done separately.
