//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/bounds.h"

namespace heph::types::internal {

auto getAsProto(const BoundsType& bounds_type) -> proto::BoundsType {
  switch (bounds_type) {
    case BoundsType::INCLUSIVE:
      return proto::BoundsType::INCLUSIVE;
    case BoundsType::LEFT_OPEN:
      return proto::BoundsType::LEFT_OPEN;
    case BoundsType::RIGHT_OPEN:
      return proto::BoundsType::RIGHT_OPEN;
    case BoundsType::OPEN:
      return proto::BoundsType::OPEN;
    default:
      throwException<InvalidParameterException>("Invalid bounds type");
      return {};  // unreachable
  }
}

auto getFromProto(const proto::BoundsType& proto_bounds_type) -> BoundsType {
  switch (proto_bounds_type) {
    case proto::BoundsType::INCLUSIVE:
      return BoundsType::INCLUSIVE;
    case proto::BoundsType::LEFT_OPEN:
      return BoundsType::LEFT_OPEN;
    case proto::BoundsType::RIGHT_OPEN:
      return BoundsType::RIGHT_OPEN;
    case proto::BoundsType::OPEN:
      return BoundsType::OPEN;
    default:
      throwException<InvalidParameterException>("Invalid bounds type");
      return {};  // unreachable
  }
}

}  // namespace heph::types::internal
