module vw.memory;
import std3rd;
import vulkan3rd;
import vma3rd;
import vw.vulkan;

namespace vw {
namespace {
MipLevel mip_level_from_size(Width width, Height height, Depth depth) {
    auto size = std::max({uint32_t(width), uint32_t(height), uint32_t(depth)});
    return static_cast<MipLevel>(uint32_t(std::log2(size)) + 1);
}
} // namespace

Allocator::Impl::Impl(std::shared_ptr<const Device> dev, VmaAllocator alloc)
    : device{std::move(dev)}
    , allocator{alloc} {}

Allocator::Impl::~Impl() {
    if (allocator != vw_VK_NULL_HANDLE) {
        vmaDestroyAllocator(allocator);
    }
}

Allocator::Allocator(std::shared_ptr<const Device> device,
                     VmaAllocator allocator)
    : m_impl{std::make_shared<Impl>(std::move(device), allocator)} {}

VmaAllocator Allocator::handle() const noexcept { return m_impl->allocator; }

std::shared_ptr<const Device> Allocator::device() const {
    return m_impl->device;
}

std::shared_ptr<const Image>
Allocator::create_image_2D(Width width, Height height, bool mipmap,
                           vk::Format format, vk::ImageUsageFlags usage) const {
    const auto mip_levels = [&] {
        if (mipmap)
            return mip_level_from_size(width, height, Depth(1));
        return MipLevel(1);
    }();

    const VkImageCreateInfo create_info =
        vk::ImageCreateInfo()
            .setExtent(vk::Extent3D(uint32_t(width), uint32_t(height), 1))
            .setMipLevels(uint32_t(mip_levels))
            .setArrayLayers(1)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setImageType(vk::ImageType::e2D)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setFormat(format)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setUsage(usage);

    VmaAllocationCreateInfo allocation_info{};
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocation allocation = nullptr;
    VkImage image = nullptr;
    vmaCreateImage(m_impl->allocator, &create_info, &allocation_info, &image,
                   &allocation, nullptr);
    auto *img = new Image(vk::Image(image), width, height, Depth(1), mip_levels,
                          format, usage);
    return std::shared_ptr<const Image>(
        img, [self = shared_from_this(), alloc = allocation](const Image *p) {
            vmaDestroyImage(self->handle(), p->handle(), alloc);
            delete p;
        });
}

AllocatorBuilder::AllocatorBuilder(std::shared_ptr<const Instance> instance,
                                   std::shared_ptr<const Device> device)
    : m_instance{std::move(instance)}
    , m_device{std::move(device)} {}

std::shared_ptr<Allocator> AllocatorBuilder::build() {
    VmaAllocatorCreateInfo info{};
    info.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT |
                 VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    info.device = m_device->handle();
    info.instance = m_instance->handle();
    info.physicalDevice = m_device->physical_device();

    VmaAllocator allocator = nullptr;
    if (vk::Result(vmaCreateAllocator(&info, &allocator)) !=
        vk::Result::eSuccess)
        std::terminate();

    return std::shared_ptr<Allocator>(new Allocator(m_device, allocator));
}

} // namespace vw
