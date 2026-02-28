export module vw.utils:alignment;
import vulkan3rd;

export namespace vw {

constexpr uint64_t align(uint64_t size, uint64_t align) {
    return (size + align - 1) & ~(align - 1);
}

constexpr vk::DeviceSize DefaultBufferAlignment = 4'096;

} // namespace vw
