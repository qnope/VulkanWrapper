export module vw.memory:buffer_usage;
import vulkan3rd;

export namespace vw {

constexpr VkBufferUsageFlags2 VertexBufferUsage = VkBufferUsageFlags2{
    vk::BufferUsageFlagBits2::eVertexBuffer |
    vk::BufferUsageFlagBits2::eTransferDst |
    vk::BufferUsageFlagBits2::eShaderDeviceAddress |
    vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR};

constexpr VkBufferUsageFlags2 IndexBufferUsage = VkBufferUsageFlags2{
    vk::BufferUsageFlagBits2::eIndexBuffer |
    vk::BufferUsageFlagBits2::eTransferDst |
    vk::BufferUsageFlagBits2::eShaderDeviceAddress |
    vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR};

constexpr VkBufferUsageFlags2 UniformBufferUsage =
    VkBufferUsageFlags2{vk::BufferUsageFlagBits2::eUniformBuffer |
                        vk::BufferUsageFlagBits2::eTransferDst |
                        vk::BufferUsageFlagBits2::eShaderDeviceAddress};

constexpr VkBufferUsageFlags2 StagingBufferUsage =
    VkBufferUsageFlags2{vk::BufferUsageFlagBits2::eTransferSrc |
                        vk::BufferUsageFlagBits2::eTransferDst |
                        vk::BufferUsageFlagBits2::eShaderDeviceAddress};

constexpr VkBufferUsageFlags2 StorageBufferUsage =
    VkBufferUsageFlags2{vk::BufferUsageFlagBits2::eStorageBuffer |
                        vk::BufferUsageFlagBits2::eTransferDst |
                        vk::BufferUsageFlagBits2::eShaderDeviceAddress};

class BufferBase;
template <typename T, bool HostVisible, VkBufferUsageFlags Flags> class Buffer;

using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;

} // namespace vw
