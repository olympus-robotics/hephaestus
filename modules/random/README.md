# README: random

## Brief

The **random** module contains functionality to generate random data.

## Detailed description

The **random** module contains generic functionality to generate random data, as it is often used in unit tests.
* a random number generator (RNG) which is pre-configured to be deterministic or not in order to enforce (non-)determinism in all tests
* a method `randomT<T>(std::mt19937_64& mt) -> T` which can be used to create a random element of a primitive, custom or container type
