#pragma once

#include "VulkanWrapper/fwd.h"
#include "VulkanWrapper/Utils/exceptions.h"
#include "VulkanWrapper/Utils/ObjectWithHandle.h"

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
