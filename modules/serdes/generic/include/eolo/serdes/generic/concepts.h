//=================================================================================================
// PIPPO © Copyright, 2015-2023. All Rights Reserved.
//
// This file is part of the RoboCool project
//=================================================================================================

#pragma once

#include <concepts>
#include <type_traits>
#include <vector>

// A set of useful "concepts" checking the required functionality.

namespace eolo::serdes {

template <typename T>
concept SerializerType = requires(T&& serializer) {
  { serializer.serialize() } -> std::same_as<std::vector<std::byte>>;
};

}  // namespace eolo::serdes
