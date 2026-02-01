#pragma once
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/BufferUsage.h"
#include <cstdint>

namespace vw::rt {

struct GeometryReference {
    uint64_t vertex_buffer_address;
    int32_t vertex_offset;
    int32_t pad0;
    uint64_t index_buffer_address;
    int32_t first_index;
    int32_t pad1;
    uint32_t material_type;
    uint32_t material_index;
};

static_assert(sizeof(GeometryReference) == 40,
              "GeometryReference must be 40 bytes for GPU compatibility");

using GeometryReferenceBuffer =
    Buffer<GeometryReference, true, StorageBufferUsage>;

} // namespace vw::rt
