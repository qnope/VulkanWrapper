#include "VulkanWrapper/Shader/ShaderCompiler.h"
#include "VulkanWrapper/Utils/Error.h"
#include <fstream>
#include <gtest/gtest.h>

namespace {

// Helper to create temporary shader files
class TempShaderFile {
public:
    TempShaderFile(const std::string &filename, const std::string &content)
        : m_path(std::filesystem::temp_directory_path() / filename) {
        std::ofstream file(m_path);
        file << content;
    }

    ~TempShaderFile() { std::filesystem::remove(m_path); }

    const std::filesystem::path &path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

// Helper to create temporary directories with shader files
class TempShaderDir {
public:
    TempShaderDir(const std::string &dirname)
        : m_path(std::filesystem::temp_directory_path() / dirname) {
        std::filesystem::create_directories(m_path);
    }

    ~TempShaderDir() { std::filesystem::remove_all(m_path); }

    void add_file(const std::string &filename, const std::string &content) {
        std::ofstream file(m_path / filename);
        file << content;
    }

    const std::filesystem::path &path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

const std::string SIMPLE_VERTEX_SHADER = R"(
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
}
)";

const std::string SIMPLE_FRAGMENT_SHADER = R"(
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
)";

const std::string COMPUTE_SHADER = R"(
#version 450

layout(local_size_x = 64) in;

layout(binding = 0) buffer Data {
    float values[];
};

void main() {
    uint idx = gl_GlobalInvocationID.x;
    values[idx] = values[idx] * 2.0;
}
)";

const std::string SHADER_WITH_INCLUDE = R"(
#version 450

#include "common.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor = vec3(PI / 3.14159, 0.0, 0.0);
}
)";

const std::string COMMON_HEADER = R"(
#define PI 3.14159265359
)";

const std::string SHADER_WITH_NESTED_INCLUDE = R"(
#version 450

#include "level1.glsl"

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(VALUE, 0.0, 0.0, 1.0);
}
)";

const std::string LEVEL1_HEADER = R"(
#include "level2.glsl"
#define VALUE (BASE_VALUE * 2.0)
)";

const std::string LEVEL2_HEADER = R"(
#define BASE_VALUE 0.5
)";

const std::string INVALID_SHADER = R"(
#version 450

void main() {
    this is invalid glsl code
}
)";

} // namespace

// Basic compilation tests
TEST(ShaderCompilerTest, CompileSimpleVertexShader) {
    vw::ShaderCompiler compiler;
    auto result = compiler.compile(SIMPLE_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);

    EXPECT_FALSE(result.spirv.empty());
    // SPIR-V magic number is 0x07230203
    EXPECT_EQ(result.spirv[0], 0x07230203u);
}

TEST(ShaderCompilerTest, CompileSimpleFragmentShader) {
    vw::ShaderCompiler compiler;
    auto result = compiler.compile(SIMPLE_FRAGMENT_SHADER, vk::ShaderStageFlagBits::eFragment);

    EXPECT_FALSE(result.spirv.empty());
    EXPECT_EQ(result.spirv[0], 0x07230203u);
}

TEST(ShaderCompilerTest, CompileComputeShader) {
    vw::ShaderCompiler compiler;
    auto result = compiler.compile(COMPUTE_SHADER, vk::ShaderStageFlagBits::eCompute);

    EXPECT_FALSE(result.spirv.empty());
    EXPECT_EQ(result.spirv[0], 0x07230203u);
}

// File compilation tests
TEST(ShaderCompilerTest, CompileFromFile) {
    TempShaderFile file("test_shader.vert", SIMPLE_VERTEX_SHADER);

    vw::ShaderCompiler compiler;
    auto result = compiler.compile_from_file(file.path());

    EXPECT_FALSE(result.spirv.empty());
    EXPECT_EQ(result.spirv[0], 0x07230203u);
}

TEST(ShaderCompilerTest, CompileFromFileWithExplicitStage) {
    TempShaderFile file("test_shader.glsl", SIMPLE_VERTEX_SHADER);

    vw::ShaderCompiler compiler;
    auto result = compiler.compile_from_file(file.path(), vk::ShaderStageFlagBits::eVertex);

    EXPECT_FALSE(result.spirv.empty());
}

// Stage detection tests
TEST(ShaderCompilerTest, DetectVertexStage) {
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.vert"),
              vk::ShaderStageFlagBits::eVertex);
}

TEST(ShaderCompilerTest, DetectFragmentStage) {
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.frag"),
              vk::ShaderStageFlagBits::eFragment);
}

TEST(ShaderCompilerTest, DetectComputeStage) {
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.comp"),
              vk::ShaderStageFlagBits::eCompute);
}

TEST(ShaderCompilerTest, DetectGeometryStage) {
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.geom"),
              vk::ShaderStageFlagBits::eGeometry);
}

TEST(ShaderCompilerTest, DetectTessControlStage) {
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.tesc"),
              vk::ShaderStageFlagBits::eTessellationControl);
}

TEST(ShaderCompilerTest, DetectTessEvalStage) {
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.tese"),
              vk::ShaderStageFlagBits::eTessellationEvaluation);
}

TEST(ShaderCompilerTest, DetectRaygenStage) {
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.rgen"),
              vk::ShaderStageFlagBits::eRaygenKHR);
}

TEST(ShaderCompilerTest, DetectMissStage) {
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.rmiss"),
              vk::ShaderStageFlagBits::eMissKHR);
}

TEST(ShaderCompilerTest, DetectClosestHitStage) {
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.rchit"),
              vk::ShaderStageFlagBits::eClosestHitKHR);
}

TEST(ShaderCompilerTest, DetectAnyHitStage) {
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.rahit"),
              vk::ShaderStageFlagBits::eAnyHitKHR);
}

TEST(ShaderCompilerTest, DetectDoubleExtension) {
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.vert.glsl"),
              vk::ShaderStageFlagBits::eVertex);
    EXPECT_EQ(vw::ShaderCompiler::detect_stage_from_extension("shader.frag.glsl"),
              vk::ShaderStageFlagBits::eFragment);
}

TEST(ShaderCompilerTest, DetectUnknownExtensionThrows) {
    EXPECT_THROW(vw::ShaderCompiler::detect_stage_from_extension("shader.txt"),
                 vw::ShaderCompilationException);
}

// Include tests
TEST(ShaderCompilerTest, CompileWithInclude) {
    TempShaderDir dir("shader_test_includes");
    dir.add_file("main.vert", SHADER_WITH_INCLUDE);
    dir.add_file("common.glsl", COMMON_HEADER);

    vw::ShaderCompiler compiler;
    compiler.add_include_path(dir.path());

    auto result = compiler.compile_from_file(dir.path() / "main.vert");

    EXPECT_FALSE(result.spirv.empty());
    EXPECT_EQ(result.includedFiles.size(), 2u); // main.vert + common.glsl
}

TEST(ShaderCompilerTest, CompileWithNestedIncludes) {
    TempShaderDir dir("shader_test_nested");
    dir.add_file("main.frag", SHADER_WITH_NESTED_INCLUDE);
    dir.add_file("level1.glsl", LEVEL1_HEADER);
    dir.add_file("level2.glsl", LEVEL2_HEADER);

    vw::ShaderCompiler compiler;
    compiler.add_include_path(dir.path());

    auto result = compiler.compile_from_file(dir.path() / "main.frag");

    EXPECT_FALSE(result.spirv.empty());
    EXPECT_EQ(result.includedFiles.size(), 3u); // main.frag + level1.glsl + level2.glsl
}

TEST(ShaderCompilerTest, MissingIncludeThrows) {
    const std::string shader_with_missing_include = R"(
#version 450
#include "nonexistent.glsl"
void main() {}
)";

    vw::ShaderCompiler compiler;
    EXPECT_THROW(compiler.compile(shader_with_missing_include, vk::ShaderStageFlagBits::eVertex),
                 vw::ShaderCompilationException);
}

// Virtual include tests
TEST(ShaderCompilerTest, VirtualIncludeWithAddInclude) {
    const std::string shader_with_virtual_include = R"(
#version 450
#include "virtual_header.glsl"
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(CUSTOM_VALUE, 0.0, 0.0, 1.0);
}
)";

    vw::ShaderCompiler compiler;
    compiler.add_include("virtual_header.glsl", "#define CUSTOM_VALUE 0.75\n");

    auto result = compiler.compile(shader_with_virtual_include, vk::ShaderStageFlagBits::eFragment);

    EXPECT_FALSE(result.spirv.empty());
}

TEST(ShaderCompilerTest, VirtualIncludeWithSetIncludes) {
    const std::string shader_with_virtual_include = R"(
#version 450
#include "constants.glsl"
#include "utils.glsl"
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(RED, GREEN, BLUE, 1.0);
}
)";

    vw::IncludeMap includes;
    includes["constants.glsl"] = "#define RED 1.0\n#define BLUE 0.0\n";
    includes["utils.glsl"] = "#define GREEN 0.5\n";

    vw::ShaderCompiler compiler;
    compiler.set_includes(std::move(includes));

    auto result = compiler.compile(shader_with_virtual_include, vk::ShaderStageFlagBits::eFragment);

    EXPECT_FALSE(result.spirv.empty());
}

// Macro tests
TEST(ShaderCompilerTest, CompileWithMacros) {
    const std::string shader_with_macro = R"(
#version 450
layout(location = 0) out vec4 outColor;
void main() {
#ifdef MY_DEFINE
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
#else
    outColor = vec4(0.0, 1.0, 0.0, 1.0);
#endif
}
)";

    vw::ShaderCompiler compiler;
    compiler.add_macro("MY_DEFINE");

    auto result = compiler.compile(shader_with_macro, vk::ShaderStageFlagBits::eFragment);
    EXPECT_FALSE(result.spirv.empty());
}

TEST(ShaderCompilerTest, CompileWithMacroValue) {
    const std::string shader_with_macro_value = R"(
#version 450
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(float(MY_VALUE) / 100.0, 0.0, 0.0, 1.0);
}
)";

    vw::ShaderCompiler compiler;
    compiler.add_macro("MY_VALUE", "50");

    auto result = compiler.compile(shader_with_macro_value, vk::ShaderStageFlagBits::eFragment);
    EXPECT_FALSE(result.spirv.empty());
}

// Error handling tests
TEST(ShaderCompilerTest, InvalidShaderThrows) {
    vw::ShaderCompiler compiler;
    EXPECT_THROW(compiler.compile(INVALID_SHADER, vk::ShaderStageFlagBits::eVertex),
                 vw::ShaderCompilationException);
}

TEST(ShaderCompilerTest, CompilationExceptionContainsLog) {
    vw::ShaderCompiler compiler;
    try {
        compiler.compile(INVALID_SHADER, vk::ShaderStageFlagBits::eVertex);
        FAIL() << "Expected ShaderCompilationException";
    } catch (const vw::ShaderCompilationException &e) {
        EXPECT_FALSE(e.compilation_log().empty());
        EXPECT_EQ(e.stage(), vk::ShaderStageFlagBits::eVertex);
    }
}

TEST(ShaderCompilerTest, NonExistentFileThrows) {
    vw::ShaderCompiler compiler;
    EXPECT_THROW(compiler.compile_from_file("/nonexistent/path/shader.vert"), vw::FileException);
}

// Builder pattern tests
TEST(ShaderCompilerTest, FluentAPILValue) {
    vw::ShaderCompiler compiler;
    compiler.add_include_path("/some/path")
        .set_target_vulkan_version(VK_API_VERSION_1_2)
        .add_macro("TEST")
        .set_generate_debug_info(true)
        .set_optimize(false);

    auto result = compiler.compile(SIMPLE_FRAGMENT_SHADER, vk::ShaderStageFlagBits::eFragment);
    EXPECT_FALSE(result.spirv.empty());
}

TEST(ShaderCompilerTest, FluentAPIRValue) {
    auto result = vw::ShaderCompiler()
                      .add_include_path("/some/path")
                      .set_target_vulkan_version(VK_API_VERSION_1_3)
                      .add_macro("TEST", "1")
                      .compile(SIMPLE_FRAGMENT_SHADER, vk::ShaderStageFlagBits::eFragment);

    EXPECT_FALSE(result.spirv.empty());
}

// Vulkan version tests
TEST(ShaderCompilerTest, CompileForVulkan10) {
    vw::ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_0);

    auto result = compiler.compile(SIMPLE_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    EXPECT_FALSE(result.spirv.empty());
}

TEST(ShaderCompilerTest, CompileForVulkan11) {
    vw::ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_1);

    auto result = compiler.compile(SIMPLE_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    EXPECT_FALSE(result.spirv.empty());
}

TEST(ShaderCompilerTest, CompileForVulkan12) {
    vw::ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_2);

    auto result = compiler.compile(SIMPLE_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    EXPECT_FALSE(result.spirv.empty());
}

TEST(ShaderCompilerTest, CompileForVulkan13) {
    vw::ShaderCompiler compiler;
    compiler.set_target_vulkan_version(VK_API_VERSION_1_3);

    auto result = compiler.compile(SIMPLE_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    EXPECT_FALSE(result.spirv.empty());
}

// Debug and optimization tests
TEST(ShaderCompilerTest, CompileWithDebugInfo) {
    vw::ShaderCompiler compiler;
    compiler.set_generate_debug_info(true);

    auto result = compiler.compile(SIMPLE_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    EXPECT_FALSE(result.spirv.empty());
}

TEST(ShaderCompilerTest, CompileWithOptimization) {
    vw::ShaderCompiler compiler;
    compiler.set_optimize(true);

    auto result = compiler.compile(SIMPLE_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    EXPECT_FALSE(result.spirv.empty());
}

// Move semantics test
TEST(ShaderCompilerTest, MoveConstruction) {
    vw::ShaderCompiler compiler1;
    compiler1.add_include_path("/test/path");

    vw::ShaderCompiler compiler2(std::move(compiler1));

    auto result = compiler2.compile(SIMPLE_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    EXPECT_FALSE(result.spirv.empty());
}

TEST(ShaderCompilerTest, MoveAssignment) {
    vw::ShaderCompiler compiler1;
    compiler1.add_include_path("/test/path");

    vw::ShaderCompiler compiler2;
    compiler2 = std::move(compiler1);

    auto result = compiler2.compile(SIMPLE_VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    EXPECT_FALSE(result.spirv.empty());
}
