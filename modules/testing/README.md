# README: testing

## Brief

The **testing** module contains helper functionality common to all tests.

## Detailed description

The **testing** module contains generic helper code for testing.
* a random number generator (RNG) which is pre-configured to be deterministic or not in order to enforce (non-)determinism in all tests
* a method `randomT<T>(std::mt19937_64& mt) -> T` which can be used to create a random element of a primitive, custom or container type
