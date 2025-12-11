#pragma once

#include "VulkanWrapper/3rd_party.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace vw {

// Forward declarations
class ShaderModule;
class Device;

/// Result of a successful shader compilation
struct ShaderCompilationResult {
    std::vector<std::uint32_t> spirv;
    std::unordered_set<std::filesystem::path> includedFiles;
};

/// Callback type for resolving include files
/// Parameters: (includePath, includerPath, isSystemInclude)
/// Returns: optional pair of (resolvedPath, fileContents)
using IncludeResolver = std::function<std::optional<std::pair<std::filesystem::path, std::string>>(
    std::string_view includePath,
    const std::filesystem::path &includerPath,
    bool isSystemInclude)>;

/// GLSL to SPIR-V compiler with include support
///
/// This class provides runtime compilation of GLSL shaders to SPIR-V bytecode.
/// It supports:
/// - All Vulkan shader stages (vertex, fragment, compute, ray tracing, etc.)
/// - #include directives with customizable resolution
/// - Automatic shader stage detection from file extensions
/// - GLSL version and Vulkan environment configuration
///
/// Example usage:
/// @code
/// auto compiler = ShaderCompiler()
///     .add_include_path("shaders/include")
///     .set_target_vulkan_version(vk::ApiVersion13);
///
/// auto result = compiler.compile_from_file("shaders/main.vert");
/// auto spirv = result.spirv;
/// @endcode
class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();

    ShaderCompiler(const ShaderCompiler &) = delete;
    ShaderCompiler &operator=(const ShaderCompiler &) = delete;
    ShaderCompiler(ShaderCompiler &&) noexcept;
    ShaderCompiler &operator=(ShaderCompiler &&) noexcept;

    /// Add an include search path for resolving #include directives
    /// @param path Directory to search for included files
    /// @return Reference to this for method chaining
    ShaderCompiler &add_include_path(const std::filesystem::path &path);

    /// Set a custom include resolver callback
    /// @param resolver Function to resolve include paths to file contents
    /// @return Reference to this for method chaining
    ShaderCompiler &set_include_resolver(IncludeResolver resolver);

    /// Set the target Vulkan API version (affects SPIR-V version)
    /// @param version Vulkan API version (e.g., vk::ApiVersion13)
    /// @return Reference to this for method chaining
    ShaderCompiler &set_target_vulkan_version(uint32_t version);

    /// Add a preprocessor macro definition
    /// @param name Macro name
    /// @param value Optional macro value (empty string for define without value)
    /// @return Reference to this for method chaining
    ShaderCompiler &add_macro(std::string_view name, std::string_view value = "");

    /// Enable or disable debug information generation
    /// @param enable True to generate debug info
    /// @return Reference to this for method chaining
    ShaderCompiler &set_generate_debug_info(bool enable);

    /// Enable or disable SPIR-V optimization
    /// @param enable True to optimize (reduces size, may improve performance)
    /// @return Reference to this for method chaining
    ShaderCompiler &set_optimize(bool enable);

    /// Compile GLSL source code to SPIR-V
    /// @param source GLSL source code
    /// @param stage Shader stage (vertex, fragment, etc.)
    /// @param sourceName Optional name for error messages
    /// @return Compilation result containing SPIR-V bytecode
    /// @throws ShaderCompilationException on compilation failure
    ShaderCompilationResult compile(std::string_view source,
                                    vk::ShaderStageFlagBits stage,
                                    std::string_view sourceName = "<source>") const;

    /// Compile a GLSL file to SPIR-V
    /// @param path Path to the GLSL source file
    /// @return Compilation result containing SPIR-V bytecode
    /// @throws ShaderCompilationException on compilation failure
    /// @throws FileException if file cannot be read
    ShaderCompilationResult compile_from_file(const std::filesystem::path &path) const;

    /// Compile a GLSL file to SPIR-V with explicit stage
    /// @param path Path to the GLSL source file
    /// @param stage Shader stage to use (overrides extension-based detection)
    /// @return Compilation result containing SPIR-V bytecode
    /// @throws ShaderCompilationException on compilation failure
    /// @throws FileException if file cannot be read
    ShaderCompilationResult compile_from_file(const std::filesystem::path &path,
                                              vk::ShaderStageFlagBits stage) const;

    /// Compile and create a ShaderModule directly
    /// @param device Vulkan device
    /// @param source GLSL source code
    /// @param stage Shader stage
    /// @param sourceName Optional name for error messages
    /// @return Created ShaderModule
    std::shared_ptr<const ShaderModule> compile_to_module(
        std::shared_ptr<const Device> device,
        std::string_view source,
        vk::ShaderStageFlagBits stage,
        std::string_view sourceName = "<source>") const;

    /// Compile a file and create a ShaderModule directly
    /// @param device Vulkan device
    /// @param path Path to the GLSL source file
    /// @return Created ShaderModule
    std::shared_ptr<const ShaderModule> compile_file_to_module(
        std::shared_ptr<const Device> device,
        const std::filesystem::path &path) const;

    /// Detect shader stage from file extension
    /// @param path File path with extension
    /// @return Detected shader stage
    /// @throws ShaderCompilationException if extension is not recognized
    static vk::ShaderStageFlagBits detect_stage_from_extension(const std::filesystem::path &path);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vw
