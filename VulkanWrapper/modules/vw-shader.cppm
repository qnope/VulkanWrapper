module;
#include "VulkanWrapper/3rd_party.h"

export module vw:shader;
import "VulkanWrapper/vw_vulkan.h";
import :core;
import :utils;
import :vulkan;
import :pipeline;

export namespace vw {

struct ShaderCompilationResult {
    std::vector<std::uint32_t> spirv;
    std::set<std::filesystem::path> includedFiles;
};

using IncludeMap = std::map<std::string, std::string>;

class ShaderCompiler {
  public:
    ShaderCompiler();
    ~ShaderCompiler();

    ShaderCompiler(const ShaderCompiler &) = delete;
    ShaderCompiler &operator=(const ShaderCompiler &) = delete;
    ShaderCompiler(ShaderCompiler &&) noexcept;
    ShaderCompiler &operator=(ShaderCompiler &&) noexcept;

    ShaderCompiler &add_include_path(const std::filesystem::path &path);

    ShaderCompiler &add_include(std::string_view name,
                                std::string_view content);

    ShaderCompiler &set_includes(IncludeMap includes);

    ShaderCompiler &set_target_vulkan_version(uint32_t version);

    ShaderCompiler &add_macro(std::string_view name,
                              std::string_view value = "");

    ShaderCompiler &set_generate_debug_info(bool enable);

    ShaderCompiler &set_optimize(bool enable);

    ShaderCompilationResult
    compile(std::string_view source, vk::ShaderStageFlagBits stage,
            std::string_view sourceName = "<source>") const;

    ShaderCompilationResult
    compile_from_file(const std::filesystem::path &path) const;

    ShaderCompilationResult
    compile_from_file(const std::filesystem::path &path,
                      vk::ShaderStageFlagBits stage) const;

    std::shared_ptr<const ShaderModule>
    compile_to_module(std::shared_ptr<const Device> device,
                      std::string_view source, vk::ShaderStageFlagBits stage,
                      std::string_view sourceName = "<source>") const;

    std::shared_ptr<const ShaderModule>
    compile_file_to_module(std::shared_ptr<const Device> device,
                           const std::filesystem::path &path) const;

    static vk::ShaderStageFlagBits
    detect_stage_from_extension(const std::filesystem::path &path);

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vw
