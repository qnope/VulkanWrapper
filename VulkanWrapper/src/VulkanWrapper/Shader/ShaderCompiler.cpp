#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Utils/Error.h"

#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include <shaderc/shaderc.hpp>

namespace vw {

namespace {

// Convert Vulkan shader stage to shaderc shader kind
shaderc_shader_kind to_shaderc_kind(vk::ShaderStageFlagBits stage) {
    switch (stage) {
    case vk::ShaderStageFlagBits::eVertex:
        return shaderc_vertex_shader;
    case vk::ShaderStageFlagBits::eTessellationControl:
        return shaderc_tess_control_shader;
    case vk::ShaderStageFlagBits::eTessellationEvaluation:
        return shaderc_tess_evaluation_shader;
    case vk::ShaderStageFlagBits::eGeometry:
        return shaderc_geometry_shader;
    case vk::ShaderStageFlagBits::eFragment:
        return shaderc_fragment_shader;
    case vk::ShaderStageFlagBits::eCompute:
        return shaderc_compute_shader;
    case vk::ShaderStageFlagBits::eRaygenKHR:
        return shaderc_raygen_shader;
    case vk::ShaderStageFlagBits::eAnyHitKHR:
        return shaderc_anyhit_shader;
    case vk::ShaderStageFlagBits::eClosestHitKHR:
        return shaderc_closesthit_shader;
    case vk::ShaderStageFlagBits::eMissKHR:
        return shaderc_miss_shader;
    case vk::ShaderStageFlagBits::eIntersectionKHR:
        return shaderc_intersection_shader;
    case vk::ShaderStageFlagBits::eCallableKHR:
        return shaderc_callable_shader;
    case vk::ShaderStageFlagBits::eTaskEXT:
        return shaderc_task_shader;
    case vk::ShaderStageFlagBits::eMeshEXT:
        return shaderc_mesh_shader;
    default:
        throw LogicException::invalid_state("Unknown shader stage");
    }
}

// Get shaderc target environment version for Vulkan API version
shaderc_env_version to_shaderc_env_version(uint32_t vulkan_version) {
    uint32_t major = VK_API_VERSION_MAJOR(vulkan_version);
    uint32_t minor = VK_API_VERSION_MINOR(vulkan_version);

    if (major >= 1) {
        if (minor >= 3) {
            return shaderc_env_version_vulkan_1_3;
        }
        if (minor >= 2) {
            return shaderc_env_version_vulkan_1_2;
        }
        if (minor >= 1) {
            return shaderc_env_version_vulkan_1_1;
        }
    }
    return shaderc_env_version_vulkan_1_0;
}

// Get SPIR-V version for Vulkan API version
shaderc_spirv_version to_spirv_version(uint32_t vulkan_version) {
    uint32_t major = VK_API_VERSION_MAJOR(vulkan_version);
    uint32_t minor = VK_API_VERSION_MINOR(vulkan_version);

    if (major >= 1) {
        if (minor >= 3) {
            return shaderc_spirv_version_1_6;
        }
        if (minor >= 2) {
            return shaderc_spirv_version_1_5;
        }
        if (minor >= 1) {
            return shaderc_spirv_version_1_3;
        }
    }
    return shaderc_spirv_version_1_0;
}

std::string read_file_contents(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::in);
    if (!file) {
        throw FileException(path, "Failed to open file for reading");
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

// Custom includer for shaderc
class VwIncluder : public shaderc::CompileOptions::IncluderInterface {
    struct IncludeData {
        std::string source_name;
        std::string content;
        shaderc_include_result result;

        IncludeData(std::string name, std::string cont)
            : source_name(std::move(name)), content(std::move(cont)) {
            result.source_name = source_name.c_str();
            result.source_name_length = source_name.size();
            result.content = content.c_str();
            result.content_length = content.size();
            result.user_data = nullptr;
        }
    };

public:
    VwIncluder(const std::vector<std::filesystem::path> &include_paths,
               const IncludeMap &virtual_includes,
               std::set<std::filesystem::path> &included_files)
        : m_include_paths(include_paths),
          m_virtual_includes(virtual_includes),
          m_included_files(included_files) {}

    shaderc_include_result *GetInclude(const char *requested_source,
                                       shaderc_include_type type,
                                       const char *requesting_source,
                                       size_t /*include_depth*/) override {
        bool is_system = (type == shaderc_include_type_standard);
        std::filesystem::path requester_path =
            requesting_source ? std::filesystem::path(requesting_source) : std::filesystem::path();

        // Try virtual includes first
        if (auto it = m_virtual_includes.find(requested_source); it != m_virtual_includes.end()) {
            m_included_files.insert(std::filesystem::path(it->first));
            return make_include_result(it->first, it->second);
        }

        // For local includes, first try relative to the requester
        if (!is_system && !requester_path.empty()) {
            auto parent = requester_path.parent_path();
            auto candidate = parent / requested_source;
            if (std::filesystem::exists(candidate)) {
                auto canonical = std::filesystem::canonical(candidate);
                m_included_files.insert(canonical);
                return make_include_result(canonical.string(), read_file_contents(canonical));
            }
        }

        // Search include paths
        for (const auto &include_path : m_include_paths) {
            auto candidate = include_path / requested_source;
            if (std::filesystem::exists(candidate)) {
                auto canonical = std::filesystem::canonical(candidate);
                m_included_files.insert(canonical);
                return make_include_result(canonical.string(), read_file_contents(canonical));
            }
        }

        // Not found - return error
        return make_include_result("", "Include file not found: " + std::string(requested_source));
    }

    void ReleaseInclude(shaderc_include_result * /*result*/) override {
        // Data owned by m_includes, released when includer is destroyed
    }

private:
    shaderc_include_result *make_include_result(std::string path, std::string content) {
        m_includes.push_back(std::make_unique<IncludeData>(std::move(path), std::move(content)));
        return &m_includes.back()->result;
    }

    const std::vector<std::filesystem::path> &m_include_paths;
    const IncludeMap &m_virtual_includes;
    std::set<std::filesystem::path> &m_included_files;
    std::vector<std::unique_ptr<IncludeData>> m_includes;
};

} // namespace

// Pimpl implementation
struct ShaderCompiler::Impl {
    shaderc::Compiler compiler;
    std::vector<std::filesystem::path> includePaths;
    IncludeMap virtualIncludes;
    uint32_t vulkanVersion = VK_API_VERSION_1_3;
    std::vector<std::pair<std::string, std::string>> macros;
    bool generateDebugInfo = false;
    bool optimize = false;
};

ShaderCompiler::ShaderCompiler() : m_impl(std::make_unique<Impl>()) {}

ShaderCompiler::~ShaderCompiler() = default;

ShaderCompiler::ShaderCompiler(ShaderCompiler &&) noexcept = default;
ShaderCompiler &ShaderCompiler::operator=(ShaderCompiler &&) noexcept = default;

ShaderCompiler &ShaderCompiler::add_include_path(const std::filesystem::path &path) {
    m_impl->includePaths.push_back(path);
    return *this;
}

ShaderCompiler &ShaderCompiler::add_include(std::string_view name, std::string_view content) {
    m_impl->virtualIncludes[std::string(name)] = std::string(content);
    return *this;
}

ShaderCompiler &ShaderCompiler::set_includes(IncludeMap includes) {
    m_impl->virtualIncludes = std::move(includes);
    return *this;
}

ShaderCompiler &ShaderCompiler::set_target_vulkan_version(uint32_t version) {
    m_impl->vulkanVersion = version;
    return *this;
}

ShaderCompiler &ShaderCompiler::add_macro(std::string_view name, std::string_view value) {
    m_impl->macros.emplace_back(std::string(name), std::string(value));
    return *this;
}

ShaderCompiler &ShaderCompiler::set_generate_debug_info(bool enable) {
    m_impl->generateDebugInfo = enable;
    return *this;
}

ShaderCompiler &ShaderCompiler::set_optimize(bool enable) {
    m_impl->optimize = enable;
    return *this;
}

ShaderCompilationResult ShaderCompiler::compile(std::string_view source,
                                                vk::ShaderStageFlagBits stage,
                                                std::string_view sourceName) const {
    shaderc::CompileOptions options;

    // Set target environment
    options.SetTargetEnvironment(shaderc_target_env_vulkan,
                                 to_shaderc_env_version(m_impl->vulkanVersion));
    options.SetTargetSpirv(to_spirv_version(m_impl->vulkanVersion));

    // Set optimization level
    if (m_impl->optimize) {
        options.SetOptimizationLevel(shaderc_optimization_level_size);
    } else {
        options.SetOptimizationLevel(shaderc_optimization_level_zero);
    }

    // Debug info
    if (m_impl->generateDebugInfo) {
        options.SetGenerateDebugInfo();
    }

    // Add macros
    for (const auto &[name, value] : m_impl->macros) {
        if (value.empty()) {
            options.AddMacroDefinition(name);
        } else {
            options.AddMacroDefinition(name, value);
        }
    }

    // Track included files
    std::set<std::filesystem::path> included_files;

    // Set includer
    options.SetIncluder(
        std::make_unique<VwIncluder>(m_impl->includePaths, m_impl->virtualIncludes, included_files));

    // Compile
    shaderc_shader_kind kind = to_shaderc_kind(stage);
    shaderc::SpvCompilationResult result = m_impl->compiler.CompileGlslToSpv(
        source.data(), source.size(), kind, std::string(sourceName).c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw ShaderCompilationException(std::string(sourceName), stage, result.GetErrorMessage());
    }

    std::vector<std::uint32_t> spirv(result.cbegin(), result.cend());

    return ShaderCompilationResult{.spirv = std::move(spirv),
                                   .includedFiles = std::move(included_files)};
}

ShaderCompilationResult ShaderCompiler::compile_from_file(const std::filesystem::path &path) const {
    return compile_from_file(path, detect_stage_from_extension(path));
}

ShaderCompilationResult ShaderCompiler::compile_from_file(const std::filesystem::path &path,
                                                          vk::ShaderStageFlagBits stage) const {
    std::string source = read_file_contents(path);

    // Add the file's directory to include paths for this compilation
    auto compiler_copy = ShaderCompiler();
    compiler_copy.m_impl->includePaths = m_impl->includePaths;
    compiler_copy.m_impl->virtualIncludes = m_impl->virtualIncludes;
    compiler_copy.m_impl->vulkanVersion = m_impl->vulkanVersion;
    compiler_copy.m_impl->macros = m_impl->macros;
    compiler_copy.m_impl->generateDebugInfo = m_impl->generateDebugInfo;
    compiler_copy.m_impl->optimize = m_impl->optimize;

    // Add parent directory as first include path
    auto parent_dir = std::filesystem::canonical(path).parent_path();
    compiler_copy.m_impl->includePaths.insert(compiler_copy.m_impl->includePaths.begin(),
                                              parent_dir);

    auto result = compiler_copy.compile(source, stage, path.string());

    // Add the main file to included files
    result.includedFiles.insert(std::filesystem::canonical(path));

    return result;
}

std::shared_ptr<const ShaderModule> ShaderCompiler::compile_to_module(
    std::shared_ptr<const Device> device,
    std::string_view source,
    vk::ShaderStageFlagBits stage,
    std::string_view sourceName) const {
    auto result = compile(source, stage, sourceName);
    return ShaderModule::create_from_spirv(std::move(device), result.spirv);
}

std::shared_ptr<const ShaderModule> ShaderCompiler::compile_file_to_module(
    std::shared_ptr<const Device> device,
    const std::filesystem::path &path) const {
    auto result = compile_from_file(path);
    return ShaderModule::create_from_spirv(std::move(device), result.spirv);
}

vk::ShaderStageFlagBits ShaderCompiler::detect_stage_from_extension(
    const std::filesystem::path &path) {
    // Map of extensions to shader stages
    // Supports both single extension (.vert) and double extension (.vert.glsl)
    static const std::map<std::string, vk::ShaderStageFlagBits> extension_map = {
        // Standard extensions
        {".vert", vk::ShaderStageFlagBits::eVertex},
        {".frag", vk::ShaderStageFlagBits::eFragment},
        {".comp", vk::ShaderStageFlagBits::eCompute},
        {".geom", vk::ShaderStageFlagBits::eGeometry},
        {".tesc", vk::ShaderStageFlagBits::eTessellationControl},
        {".tese", vk::ShaderStageFlagBits::eTessellationEvaluation},
        // Ray tracing extensions
        {".rgen", vk::ShaderStageFlagBits::eRaygenKHR},
        {".rahit", vk::ShaderStageFlagBits::eAnyHitKHR},
        {".rchit", vk::ShaderStageFlagBits::eClosestHitKHR},
        {".rmiss", vk::ShaderStageFlagBits::eMissKHR},
        {".rint", vk::ShaderStageFlagBits::eIntersectionKHR},
        {".rcall", vk::ShaderStageFlagBits::eCallableKHR},
        // Mesh shader extensions
        {".task", vk::ShaderStageFlagBits::eTaskEXT},
        {".mesh", vk::ShaderStageFlagBits::eMeshEXT},
    };

    std::string filename = path.filename().string();

    // Try to find stage extension in filename (handles .vert.glsl, etc.)
    for (const auto &[ext, stage] : extension_map) {
        // Check if extension appears in filename (before any secondary extension)
        auto pos = filename.find(ext);
        if (pos != std::string::npos) {
            // Make sure it's at the end or followed by another extension
            auto after = pos + ext.size();
            if (after == filename.size() || filename[after] == '.') {
                return stage;
            }
        }
    }

    // Try direct extension
    std::string extension = path.extension().string();
    auto it = extension_map.find(extension);
    if (it != extension_map.end()) {
        return it->second;
    }

    throw ShaderCompilationException(path.string(), vk::ShaderStageFlagBits::eVertex,
                                     "Cannot detect shader stage from extension: " + extension);
}

} // namespace vw
