#pragma once
#include "VulkanWrapper/3rd_party.h"

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

namespace vw {

class ShaderModule : public ObjectWithUniqueHandle<vk::UniqueShaderModule> {
  public:
    static std::shared_ptr<const ShaderModule>
    create_from_spirv(std::shared_ptr<const Device> device,
                      std::span<const std::uint32_t> spirv);

    static std::shared_ptr<const ShaderModule>
    create_from_spirv_file(std::shared_ptr<const Device> device,
                           const std::filesystem::path &path);

  private:
    using ObjectWithUniqueHandle<
        vk::UniqueShaderModule>::ObjectWithUniqueHandle;
};
} // namespace vw
