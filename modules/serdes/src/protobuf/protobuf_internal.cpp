//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/serdes/protobuf/protobuf_internal.h"

namespace eolo::serdes::protobuf::internal {
auto buildFileDescriptorSet(const google::protobuf::Descriptor* toplevel_descriptor)
    -> google::protobuf::FileDescriptorSet {
  google::protobuf::FileDescriptorSet fd_set;
  std::queue<const google::protobuf::FileDescriptor*> to_add;
  to_add.push(toplevel_descriptor->file());
  std::unordered_set<std::string> seen_dependencies;

  while (!to_add.empty()) {
    const google::protobuf::FileDescriptor* next = to_add.front();
    to_add.pop();
    next->CopyTo(fd_set.add_file());
    for (int i = 0; i < next->dependency_count(); ++i) {
      const auto& dep = next->dependency(i);
      if (seen_dependencies.find(dep->name()) == seen_dependencies.end()) {
        seen_dependencies.insert(dep->name());
        to_add.push(dep);
      }
    }
  }

  return fd_set;
}
}  // namespace eolo::serdes::protobuf::internal
