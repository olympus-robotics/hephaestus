//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <Eigen/Dense>

#include "hephaestus/examples/types/proto/geometry.pb.h"
#include "hephaestus/utils/exception.h"

namespace heph::examples::types {

template <class TypeT, int ROWS, int COLS, class ProtoMatrixT>
inline void toProto(ProtoMatrixT& proto_matrix, const Eigen::Matrix<TypeT, ROWS, COLS>& matrix) {
  proto_matrix.set_cols(static_cast<uint32_t>(matrix.cols()));
  proto_matrix.set_rows(static_cast<uint32_t>(matrix.rows()));

  auto* proto_data = proto_matrix.mutable_data();
  proto_data->Resize(static_cast<int>(matrix.size()), TypeT{});
  Eigen::Map<Eigen::Matrix<TypeT, ROWS, COLS>> map(proto_data->mutable_data(), matrix.rows(), matrix.cols());
  map = matrix;
}

template <class TypeT, int ROWS, int COLS, class ProtoMatrixT>
inline void fromProto(const ProtoMatrixT& proto_matrix, Eigen::Matrix<TypeT, ROWS, COLS>& matrix) {
  throwExceptionIf<InvalidDataException>((ROWS != -1 && static_cast<int>(proto_matrix.rows()) != ROWS) ||
                                             (COLS != -1 && static_cast<int>(proto_matrix.cols()) != COLS),
                                         "Cannot convert Protobuf struct to Eigen::Map: invalid dimensions");

  throwExceptionIf<InvalidDataException>(proto_matrix.rows() * proto_matrix.cols() !=
                                             static_cast<uint32_t>(proto_matrix.data_size()),
                                         "Rows and cols count don't match with data size");

  const Eigen::Map<const Eigen::Matrix<TypeT, ROWS, COLS>> map(proto_matrix.data().data(),
                                                               proto_matrix.rows(), proto_matrix.cols());
  matrix = map;
}

template <class TypeT, class ProtoVectorT>
inline void toProto(ProtoVectorT& proto_vec, const Eigen::VectorX<TypeT>& vec) {
  auto* proto_data = proto_vec.mutable_data();
  proto_data->Resize(static_cast<int>(vec.size()), TypeT{});
  Eigen::Map<Eigen::VectorX<TypeT>> map(proto_data->mutable_data(), vec.size());
  map = vec;
}

template <class TypeT, class ProtoVectorT>
inline auto fromProto(const ProtoVectorT& proto_vec, Eigen::VectorX<TypeT>& vec) {
  const Eigen::Map<const Eigen::VectorX<TypeT>> map(proto_vec.data().data(), proto_vec.data_size());
  vec = map;
}

template <class TypeT, class ProtoVector2>
inline void toProto(ProtoVector2& proto_vec, const Eigen::Vector2<TypeT>& vec) {
  proto_vec.set_x(vec.x());
  proto_vec.set_y(vec.y());
}

template <class TypeT, class ProtoVector2>
inline auto fromProto(const ProtoVector2& proto_vec, Eigen::Vector2<TypeT>& vec) {
  vec.x() = proto_vec.x();
  vec.y() = proto_vec.y();
}

template <class TypeT, class ProtoVector3>
inline void toProto(ProtoVector3& proto_vec, const Eigen::Vector3<TypeT>& vec) {
  proto_vec.set_x(vec.x());
  proto_vec.set_y(vec.y());
  proto_vec.set_z(vec.z());
}

template <class TypeT, class ProtoVector3>
inline auto fromProto(const ProtoVector3& proto_vec, Eigen::Vector3<TypeT>& vec) {
  vec.x() = proto_vec.x();
  vec.y() = proto_vec.y();
  vec.z() = proto_vec.z();
}

inline void toProto(proto::Quaterniond& proto_q, const Eigen::Quaterniond& q) {
  proto_q.set_x(q.x());
  proto_q.set_y(q.y());
  proto_q.set_z(q.z());
  proto_q.set_w(q.w());
}

inline void fromProto(const proto::Quaterniond& proto_q, Eigen::Quaterniond& q) {
  q.x() = proto_q.x();
  q.y() = proto_q.y();
  q.z() = proto_q.z();
  q.w() = proto_q.w();
}

}  // namespace heph::examples::types
