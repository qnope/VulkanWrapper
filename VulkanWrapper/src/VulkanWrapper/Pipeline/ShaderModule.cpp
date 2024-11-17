#include "VulkanWrapper/Pipeline/ShaderModule.h"

#include "VulkanWrapper/Vulkan/Device.h"
#include <fstream>

namespace vw {

static std::vector<uint32_t> readSpirVFile(const std::filesystem::path &path) {
    constexpr auto flags =
        std::ios_base::ate | std::ios_base::in | std::ios_base::binary;

    std::ifstream in(path, flags);

    if (!in)
        throw SpirVFileNotFoundException{std::source_location::current()};

    const std::size_t size = in.tellg();

    if (size == 0 || size % sizeof(std::uint32_t) != 0)
        throw SpirVIncorrectSizeException{std::source_location::current()};

    std::vector<std::uint32_t> result(size / sizeof(std::uint32_t));

    in.seekg(0);

    in.read(reinterpret_cast<char *>(result.data()), size);

    return result;
}

ShaderModule
ShaderModule::createFromSpirV(Device &device,
                              const std::vector<std::uint32_t> &spirV) {
    auto info = vk::ShaderModuleCreateInfo()
                    .setCodeSize(spirV.size() * sizeof(std::uint32_t))
                    .setCode(spirV);
    auto [result, module] = device.handle().createShaderModuleUnique(info);

    if (result != vk::Result::eSuccess)
        throw SpirVInvalidException{std::source_location::current()};

    return ShaderModule{std::move(module)};
}

ShaderModule
ShaderModule::createFromSpirVFile(Device &device,
                                  const std::filesystem::path &path) {
    const auto spirV = readSpirVFile(path);
    return createFromSpirV(device, spirV);
}

} // namespace vw
