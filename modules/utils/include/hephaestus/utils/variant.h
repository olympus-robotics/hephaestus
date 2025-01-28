//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

namespace heph {

/// A helper struct which allows you to easily branch on the type of alternative held by a variant:
///
///  std::visit( heph::Overloads {
///    [](const &FirstAlternative& value) {
///      // Code to be run if variant holds a value of type FirstAlternative.
///    },
///    [](const &SecondAlternative& value) {
///      // Code to be run if variant holds a value of type SecondAlternative.
///    },
///  }, variant);
///
template <class... Ts>
struct Overloads : Ts... {
  using Ts::operator()...;
};

}  // namespace heph
