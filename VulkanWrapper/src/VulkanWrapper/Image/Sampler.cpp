#include "VulkanWrapper/Image/Sampler.h"

#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Vulkan/Device.h"

vw::SamplerBuilder::SamplerBuilder(std::shared_ptr<const Device> device)
    : m_device{std::move(device)} {
    m_info = vk::SamplerCreateInfo()
                 .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                 .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                 .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                 .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                 .setMinFilter(vk::Filter::eLinear)
                 .setMagFilter(vk::Filter::eLinear)
                 .setMinLod(0.0)
                 .setMaxLod(VK_LOD_CLAMP_NONE);
}

std::shared_ptr<const vw::Sampler> vw::SamplerBuilder::build() {
    auto sampler = check_vk(m_device->handle().createSamplerUnique(m_info),
                            "Failed to create sampler");
    return std::make_shared<Sampler>(std::move(sampler));
}
