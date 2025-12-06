#include "VulkanWrapper/Pipeline/ShaderModule.h"

#include "VulkanWrapper/Vulkan/Device.h"
#include <fstream>

namespace vw {

static std::vector<uint32_t> readSpirVFile(const std::filesystem::path &path) {
    constexpr auto flags =
        std::ios_base::ate | std::ios_base::in | std::ios_base::binary;

    std::ifstream in(path, flags);

    if (!in) {
        std::cout << path << " not found";
        throw SpirVFileNotFoundException{std::source_location::current()};
    }

    const auto size = in.tellg();

    if (size == 0 || size % sizeof(std::uint32_t) != 0) {
        throw SpirVIncorrectSizeException{std::source_location::current()};
    }

    std::vector<std::uint32_t> result(size / sizeof(std::uint32_t));

    in.seekg(0);

    in.read(reinterpret_cast<char *>(result.data()), size);

    return result;
}

std::shared_ptr<const ShaderModule>
ShaderModule::create_from_spirv(std::shared_ptr<const Device> device,
                                std::span<const std::uint32_t> spirV) {
    auto info = vk::ShaderModuleCreateInfo()
                    .setCodeSize(spirV.size() * sizeof(std::uint32_t))
                    .setCode(spirV);
    auto [result, module] = device->handle().createShaderModuleUnique(info);

    if (result != vk::Result::eSuccess) {
        throw SpirVInvalidException{std::source_location::current()};
    }

    return std::make_shared<ShaderModule>(std::move(module));
}

std::shared_ptr<const ShaderModule>
ShaderModule::create_from_spirv_file(std::shared_ptr<const Device> device,
                                     const std::filesystem::path &path) {
    const auto spirV = readSpirVFile(path);
    return create_from_spirv(std::move(device), spirV);
}

} // namespace vw
