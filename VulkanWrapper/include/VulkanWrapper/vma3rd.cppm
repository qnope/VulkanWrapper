module;

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

export module vma3rd;

// VMA types and functions
export using ::VmaAllocator;
export using ::VmaAllocation;
export using ::VmaAllocationCreateInfo;
export using ::VmaAllocatorCreateInfo;
export using ::VmaAllocationInfo;
export using ::VmaMemoryUsage;
export using ::VmaAllocationCreateFlagBits;
export using ::VmaAllocatorCreateFlagBits;
export using ::vmaCreateAllocator;
export using ::vmaDestroyAllocator;
export using ::vmaCreateImage;
export using ::vmaDestroyImage;
export using ::vmaCreateBufferWithAlignment;
export using ::vmaDestroyBuffer;
export using ::vmaCopyMemoryToAllocation;
export using ::vmaCopyAllocationToMemory;
export using ::VMA_MEMORY_USAGE_AUTO;
export using ::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
export using ::VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
export using ::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
