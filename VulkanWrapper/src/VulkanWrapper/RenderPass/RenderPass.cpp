#include "VulkanWrapper/RenderPass/RenderPass.h"

#include <stdexcept>

namespace vw {

RenderPass::RenderPass(std::shared_ptr<Device> device,
                       std::shared_ptr<Allocator> allocator)
    : m_device(std::move(device))
    , m_allocator(std::move(allocator)) {}

const CachedImage &
RenderPass::get_or_create_image(Slot slot, Width width, Height height,
                                size_t frame_index, vk::Format format,
                                vk::ImageUsageFlags usage) {
    ImageKey key{slot, static_cast<uint32_t>(width),
                 static_cast<uint32_t>(height), frame_index};

    // Check if image already exists
    auto it = m_image_cache.find(key);
    if (it != m_image_cache.end()) {
        return it->second;
    }

    // Remove images with different dimensions to avoid memory overhead
    std::erase_if(m_image_cache, [&](const auto &entry) {
        return entry.first.slot == slot &&
               (entry.first.width != key.width ||
                entry.first.height != key.height);
    });

    // Create new image
    auto image =
        m_allocator->create_image_2D(width, height, false, format, usage);

    auto view = ImageViewBuilder(m_device, image)
                    .setImageType(vk::ImageViewType::e2D)
                    .build();

    auto [inserted_it, success] = m_image_cache.emplace(
        key, CachedImage{std::move(image), std::move(view)});

    return inserted_it->second;
}

std::vector<std::pair<Slot, CachedImage>>
RenderPass::result_images() const {
    // Collect unique slots, returning the most recent entry per slot
    std::map<Slot, CachedImage> per_slot;
    for (const auto &[key, cached] : m_image_cache) {
        per_slot.insert_or_assign(key.slot, cached);
    }

    std::vector<std::pair<Slot, CachedImage>> result;
    result.reserve(per_slot.size());
    for (auto &[slot, cached] : per_slot) {
        result.emplace_back(slot, std::move(cached));
    }
    return result;
}

void RenderPass::set_input(Slot slot, CachedImage image) {
    m_inputs.insert_or_assign(slot, std::move(image));
}

const CachedImage &RenderPass::get_input(Slot slot) const {
    auto it = m_inputs.find(slot);
    if (it == m_inputs.end()) {
        throw std::runtime_error(
            "RenderPass::get_input: slot not wired");
    }
    return it->second;
}

} // namespace vw
