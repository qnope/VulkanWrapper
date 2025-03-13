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
    static std::shared_ptr<ShaderModule>
    create_from_spirv(const Device &device,
                      std::span<const std::uint32_t> spirv);

    static std::shared_ptr<ShaderModule>
    create_from_spirv_file(const Device &device,
                           const std::filesystem::path &path);

  private:
    using ObjectWithUniqueHandle<
        vk::UniqueShaderModule>::ObjectWithUniqueHandle;
};
} // namespace vw
