//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/bounds.h"

#include "hephaestus/types//bounds.h"
#include "hephaestus/types/proto/bounds.pb.h"
#include "hephaestus/utils/exception.h"

namespace heph::types::internal {

auto getAsProto(const BoundsType& bounds_type) -> proto::BoundsType {
  switch (bounds_type) {
    case BoundsType::INCLUSIVE:
      return proto::BoundsType::BOUNDS_TYPE_INCLUSIVE;
    case BoundsType::LEFT_OPEN:
      return proto::BoundsType::BOUNDS_TYPE_LEFT_OPEN;
    case BoundsType::RIGHT_OPEN:
      return proto::BoundsType::BOUNDS_TYPE_RIGHT_OPEN;
    case BoundsType::OPEN:
      return proto::BoundsType::BOUNDS_TYPE_OPEN;
    default:
      throwException<InvalidParameterException>("Invalid bounds type");
      return {};  // unreachable
  }
}

auto getFromProto(const proto::BoundsType& proto_bounds_type) -> BoundsType {
  switch (proto_bounds_type) {
    case proto::BoundsType::BOUNDS_TYPE_INCLUSIVE:
      return BoundsType::INCLUSIVE;
    case proto::BoundsType::BOUNDS_TYPE_LEFT_OPEN:
      return BoundsType::LEFT_OPEN;
    case proto::BoundsType::BOUNDS_TYPE_RIGHT_OPEN:
      return BoundsType::RIGHT_OPEN;
    case proto::BoundsType::BOUNDS_TYPE_OPEN:
      return BoundsType::OPEN;
    default:
      throwException<InvalidParameterException>("Invalid bounds type");
      return {};  // unreachable
  }
}

}  // namespace heph::types::internal
