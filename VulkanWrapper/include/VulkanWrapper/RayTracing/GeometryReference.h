#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Memory/Buffer.h"
#include "VulkanWrapper/Memory/BufferUsage.h"
#include <cstdint>

namespace vw::rt {

#pragma pack(push, 1)
struct GeometryReference {
    uint64_t vertex_buffer_address;
    uint64_t index_buffer_address;
    int32_t vertex_offset;
    int32_t first_index;
    uint32_t material_type;
    uint32_t _padding = 0;
    uint64_t material_address;
    glm::mat4 matrix;
};
#pragma pack(pop)

static_assert(sizeof(GeometryReference) == 104,
              "GeometryReference must be 104 bytes for GPU scalar layout");

using GeometryReferenceBuffer =
    Buffer<GeometryReference, true, StorageBufferUsage>;

} // namespace vw::rt
