# README: random

## Brief

The **format** module contains functionality format types.

## Detailed description

The **format** module defines several toString methods as well as a generic_formatter.
The generic_formatter is intended to use for types, so that we do not need to implement formatting individually.
Use like:
```cpp
// By including the generic_formatter a fmt::formatter will be declared for any type you define
#include "hephaestus/format/generic_formatter.h" // NOLINT(misc-include-cleaner)

struct MySubType{
  std::string a="answer to everything";
  int b=42;
};

struct MyType{
  int c=3;
  MySubType sub{};
  // ChronoType timestamp;
};

int main() {
  MyType x{};
  // Will print format to yaml and print
  fmt::println("{}", x);
}
```
