#pragma once

#include "VulkanWrapper/Core/fwd.h"
#include "VulkanWrapper/Core/Utils/exceptions.h"
#include "VulkanWrapper/Core/Vulkan/ObjectWithHandle.h"

namespace vw {
using SpirVFileNotFoundException =
    vw::TaggedException<struct SpirVFileNotFoundTag>;

using SpirVIncorrectSizeException =
    vw::TaggedException<struct SpirVIncorrectSizeTag>;

using SpirVInvalidException = vw::TaggedException<struct SpirVInvalidTag>;

class ShaderModule : public ObjectWithUniqueHandle<vk::UniqueShaderModule> {
  public:
    static ShaderModule
    createFromSpirV(Device &device, const std::vector<std::uint32_t> &spirV);
    static ShaderModule createFromSpirVFile(Device &device,
                                            const std::filesystem::path &path);

  private:
    using ObjectWithUniqueHandle<
        vk::UniqueShaderModule>::ObjectWithUniqueHandle;
};
} // namespace vw
