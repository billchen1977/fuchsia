// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/mozart/src/scene_manager/resources/shapes/mesh_shape.h"

#include "apps/mozart/src/scene_manager/engine/session.h"

namespace scene_manager {

const ResourceTypeInfo MeshShape::kTypeInfo = {
    ResourceType::kShape | ResourceType::kMesh, "MeshShape"};

MeshShape::MeshShape(Session* session, mozart::ResourceId id)
    : Shape(session, id, MeshShape::kTypeInfo) {}

escher::Object MeshShape::GenerateRenderObject(
    const escher::mat4& transform,
    const escher::MaterialPtr& material) {
  return escher::Object(transform, mesh_, material);
}

bool MeshShape::GetIntersection(const escher::ray4& ray,
                                float* out_distance) const {
  // TODO(MZ-274): implement ray-intersection.
  return false;
}

bool MeshShape::BindBuffers(BufferPtr index_buffer,
                            mozart2::MeshIndexFormat index_format,
                            uint64_t index_offset,
                            uint32_t index_count,
                            BufferPtr vertex_buffer,
                            const mozart2::MeshVertexFormatPtr& vertex_format,
                            uint64_t vertex_offset,
                            uint32_t vertex_count,
                            escher::BoundingBox bounding_box) {
  if (index_format != mozart2::MeshIndexFormat::kUint32) {
    // TODO(MZ-275): only 32-bit indices are supported.
    session()->error_reporter()->ERROR() << "BindBuffers::Update(): "
                                            "TODO(MZ-275): only 32-bit indices "
                                            "are supported.";
    return false;
  }
  escher::MeshSpec spec;
  switch (vertex_format->position_type) {
    case mozart2::ValueType::kVector2:
      spec.flags |= escher::MeshAttribute::kPosition2D;
      break;
    case mozart2::ValueType::kVector3:
      spec.flags |= escher::MeshAttribute::kPosition3D;
      break;
    default:
      session()->error_reporter()->ERROR()
          << "MeshShape::BindBuffers(): bad vertex position format.";
      return false;
  }
  switch (vertex_format->normal_type) {
    case mozart2::ValueType::kNone:
      break;
    default:
      session()->error_reporter()->ERROR()
          << "MeshShape::BindBuffers(): bad vertex normal format.";
      return false;
  }
  switch (vertex_format->tex_coord_type) {
    case mozart2::ValueType::kVector2:
      spec.flags |= escher::MeshAttribute::kUV;
      break;
    case mozart2::ValueType::kNone:
      break;
    default:
      session()->error_reporter()->ERROR()
          << "MeshShape::BindBuffers(): bad vertex tex-coord format.";
      return false;
  }
  auto escher = session()->escher();
  mesh_ = ftl::MakeRefCounted<escher::Mesh>(
      escher->resource_recycler(), spec, bounding_box, vertex_count,
      index_count, vertex_buffer->escher_buffer(),
      index_buffer->escher_buffer(), vertex_offset, index_offset);

  index_buffer_ = std::move(index_buffer);
  vertex_buffer_ = std::move(vertex_buffer);

  return true;
}

}  // namespace scene_manager
