#pragma once
#include "VulkanWrapper/3rd_party.h"

namespace vw {
constexpr uint64_t align(uint64_t size, uint64_t align) {
    return (size + align - 1) & ~(align - 1);
}

constexpr vk::DeviceSize DefaultBufferAlignment = 4'096;

} // namespace vw
