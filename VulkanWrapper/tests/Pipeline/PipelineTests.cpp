#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Descriptors/DescriptorSetLayout.h"
#include "VulkanWrapper/Descriptors/Vertex.h"
#include "VulkanWrapper/Pipeline/Pipeline.h"
#include "VulkanWrapper/Pipeline/PipelineLayout.h"
#include "VulkanWrapper/Pipeline/ShaderModule.h"
#include "VulkanWrapper/Utils/Error.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace {
// Minimal valid SPIR-V vertex shader that does nothing
// This is the binary form of a minimal shader that just has a void main() {}
// OpCapability Shader
// OpMemoryModel Logical GLSL450
// OpEntryPoint Vertex %main "main"
// OpName %main "main"
// %void = OpTypeVoid
// %func_type = OpTypeFunction %void
// %main = OpFunction %void None %func_type
// %entry = OpLabel
// OpReturn
// OpFunctionEnd

const std::vector<std::uint32_t> kMinimalVertexShaderSpirV = {
    0x07230203, // Magic number
    0x00010000, // Version 1.0
    0x00000000, // Generator (unknown)
    0x00000008, // Bound (highest ID + 1)
    0x00000000, // Reserved
    // OpCapability Shader
    0x00020011, 0x00000001,
    // OpMemoryModel Logical GLSL450
    0x0003000e, 0x00000000, 0x00000001,
    // OpEntryPoint Vertex %1 "main"
    0x0005000f, 0x00000000, 0x00000001, 0x6e69616d, 0x00000000,
    // OpName %1 "main"
    0x00040005, 0x00000001, 0x6e69616d, 0x00000000,
    // %2 = OpTypeVoid
    0x00020013, 0x00000002,
    // %3 = OpTypeFunction %2
    0x00030021, 0x00000003, 0x00000002,
    // %1 = OpFunction %2 None %3
    0x00050036, 0x00000002, 0x00000001, 0x00000000, 0x00000003,
    // %4 = OpLabel
    0x000200f8, 0x00000004,
    // OpReturn
    0x000100fd,
    // OpFunctionEnd
    0x00010038};

const std::vector<std::uint32_t> kMinimalFragmentShaderSpirV = {
    0x07230203, // Magic number
    0x00010000, // Version 1.0
    0x00000000, // Generator (unknown)
    0x00000008, // Bound (highest ID + 1)
    0x00000000, // Reserved
    // OpCapability Shader
    0x00020011, 0x00000001,
    // OpMemoryModel Logical GLSL450
    0x0003000e, 0x00000000, 0x00000001,
    // OpEntryPoint Fragment %1 "main"
    0x0005000f, 0x00000004, 0x00000001, 0x6e69616d, 0x00000000,
    // OpExecutionMode %1 OriginUpperLeft
    0x00030010, 0x00000001, 0x00000007,
    // OpName %1 "main"
    0x00040005, 0x00000001, 0x6e69616d, 0x00000000,
    // %2 = OpTypeVoid
    0x00020013, 0x00000002,
    // %3 = OpTypeFunction %2
    0x00030021, 0x00000003, 0x00000002,
    // %1 = OpFunction %2 None %3
    0x00050036, 0x00000002, 0x00000001, 0x00000000, 0x00000003,
    // %4 = OpLabel
    0x000200f8, 0x00000004,
    // OpReturn
    0x000100fd,
    // OpFunctionEnd
    0x00010038};

std::filesystem::path createTempSpirVFile(
    const std::vector<std::uint32_t> &spirv) {
    auto path = std::filesystem::temp_directory_path() / "test_shader.spv";
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(spirv.data()),
               spirv.size() * sizeof(std::uint32_t));
    return path;
}
} // namespace

// PipelineLayoutBuilder Tests

TEST(PipelineLayoutBuilderTest, BuildEmptyLayout) {
    auto &gpu = vw::tests::create_gpu();
    auto layout = vw::PipelineLayoutBuilder(gpu.device).build();

    EXPECT_NE(layout.handle(), nullptr);
}

TEST(PipelineLayoutBuilderTest, BuildWithSingleDescriptorSetLayout) {
    auto &gpu = vw::tests::create_gpu();
    auto setLayout = vw::DescriptorSetLayoutBuilder(gpu.device)
                         .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                         .build();

    auto layout = vw::PipelineLayoutBuilder(gpu.device)
                      .with_descriptor_set_layout(setLayout)
                      .build();

    EXPECT_NE(layout.handle(), nullptr);
}

TEST(PipelineLayoutBuilderTest, BuildWithMultipleDescriptorSetLayouts) {
    auto &gpu = vw::tests::create_gpu();
    auto setLayout1 = vw::DescriptorSetLayoutBuilder(gpu.device)
                          .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                          .build();
    auto setLayout2 = vw::DescriptorSetLayoutBuilder(gpu.device)
                          .with_combined_image(vk::ShaderStageFlagBits::eFragment, 2)
                          .build();

    auto layout = vw::PipelineLayoutBuilder(gpu.device)
                      .with_descriptor_set_layout(setLayout1)
                      .with_descriptor_set_layout(setLayout2)
                      .build();

    EXPECT_NE(layout.handle(), nullptr);
}

TEST(PipelineLayoutBuilderTest, BuildWithPushConstantRange) {
    auto &gpu = vw::tests::create_gpu();
    auto pushConstantRange =
        vk::PushConstantRange()
            .setStageFlags(vk::ShaderStageFlagBits::eVertex)
            .setOffset(0)
            .setSize(sizeof(glm::mat4));

    auto layout = vw::PipelineLayoutBuilder(gpu.device)
                      .with_push_constant_range(pushConstantRange)
                      .build();

    EXPECT_NE(layout.handle(), nullptr);
}

TEST(PipelineLayoutBuilderTest, BuildWithMultiplePushConstantRanges) {
    auto &gpu = vw::tests::create_gpu();
    auto pushConstantRange1 =
        vk::PushConstantRange()
            .setStageFlags(vk::ShaderStageFlagBits::eVertex)
            .setOffset(0)
            .setSize(sizeof(glm::mat4));
    auto pushConstantRange2 =
        vk::PushConstantRange()
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            .setOffset(sizeof(glm::mat4))
            .setSize(sizeof(glm::vec4));

    auto layout = vw::PipelineLayoutBuilder(gpu.device)
                      .with_push_constant_range(pushConstantRange1)
                      .with_push_constant_range(pushConstantRange2)
                      .build();

    EXPECT_NE(layout.handle(), nullptr);
}

TEST(PipelineLayoutBuilderTest, BuildWithDescriptorSetAndPushConstants) {
    auto &gpu = vw::tests::create_gpu();
    auto setLayout = vw::DescriptorSetLayoutBuilder(gpu.device)
                         .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                         .build();
    auto pushConstantRange =
        vk::PushConstantRange()
            .setStageFlags(vk::ShaderStageFlagBits::eVertex)
            .setOffset(0)
            .setSize(sizeof(glm::mat4));

    auto layout = vw::PipelineLayoutBuilder(gpu.device)
                      .with_descriptor_set_layout(setLayout)
                      .with_push_constant_range(pushConstantRange)
                      .build();

    EXPECT_NE(layout.handle(), nullptr);
}

TEST(PipelineLayoutBuilderTest, FluentApiChaining) {
    auto &gpu = vw::tests::create_gpu();
    auto setLayout1 = vw::DescriptorSetLayoutBuilder(gpu.device)
                          .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex, 1)
                          .build();
    auto setLayout2 = vw::DescriptorSetLayoutBuilder(gpu.device)
                          .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
                          .build();
    auto pushConstant =
        vk::PushConstantRange()
            .setStageFlags(vk::ShaderStageFlagBits::eAll)
            .setOffset(0)
            .setSize(128);

    // Verify chaining works
    auto layout = vw::PipelineLayoutBuilder(gpu.device)
                      .with_descriptor_set_layout(setLayout1)
                      .with_descriptor_set_layout(setLayout2)
                      .with_push_constant_range(pushConstant)
                      .build();

    EXPECT_NE(layout.handle(), nullptr);
}

// ShaderModule Tests

TEST(ShaderModuleTest, CreateFromSpirV) {
    auto &gpu = vw::tests::create_gpu();
    auto shader = vw::ShaderModule::create_from_spirv(gpu.device,
                                                      kMinimalVertexShaderSpirV);

    EXPECT_NE(shader->handle(), nullptr);
}

TEST(ShaderModuleTest, CreateFragmentShaderFromSpirV) {
    auto &gpu = vw::tests::create_gpu();
    auto shader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    EXPECT_NE(shader->handle(), nullptr);
}

TEST(ShaderModuleTest, CreateFromSpirVFile) {
    auto &gpu = vw::tests::create_gpu();
    auto tempPath = createTempSpirVFile(kMinimalVertexShaderSpirV);

    auto shader =
        vw::ShaderModule::create_from_spirv_file(gpu.device, tempPath);

    EXPECT_NE(shader->handle(), nullptr);

    std::filesystem::remove(tempPath);
}

TEST(ShaderModuleTest, FileNotFoundThrows) {
    auto &gpu = vw::tests::create_gpu();
    EXPECT_THROW(
        vw::ShaderModule::create_from_spirv_file(gpu.device, "/nonexistent/shader.spv"),
        vw::FileException);
}

TEST(ShaderModuleTest, EmptyFileThrows) {
    auto &gpu = vw::tests::create_gpu();
    auto tempPath = std::filesystem::temp_directory_path() / "empty_shader.spv";
    std::ofstream file(tempPath);
    file.close();

    EXPECT_THROW(vw::ShaderModule::create_from_spirv_file(gpu.device, tempPath),
                 vw::FileException);

    std::filesystem::remove(tempPath);
}

TEST(ShaderModuleTest, InvalidSizeFileThrows) {
    auto &gpu = vw::tests::create_gpu();
    auto tempPath =
        std::filesystem::temp_directory_path() / "invalid_size_shader.spv";
    std::ofstream file(tempPath, std::ios::binary);
    // Write 3 bytes (not multiple of 4)
    const char data[] = {0x01, 0x02, 0x03};
    file.write(data, 3);
    file.close();

    EXPECT_THROW(vw::ShaderModule::create_from_spirv_file(gpu.device, tempPath),
                 vw::FileException);

    std::filesystem::remove(tempPath);
}

TEST(ShaderModuleTest, MultipleShaderCreation) {
    auto &gpu = vw::tests::create_gpu();
    auto shader1 = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalVertexShaderSpirV);
    auto shader2 = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    EXPECT_NE(shader1->handle(), nullptr);
    EXPECT_NE(shader2->handle(), nullptr);
    EXPECT_NE(shader1->handle(), shader2->handle());
}

// GraphicsPipelineBuilder Tests

TEST(GraphicsPipelineBuilderTest, CreateMinimalPipeline) {
    auto &gpu = vw::tests::create_gpu();
    auto pipelineLayout = vw::PipelineLayoutBuilder(gpu.device).build();
    auto vertShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalVertexShaderSpirV);
    auto fragShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    auto pipeline = vw::GraphicsPipelineBuilder(gpu.device, std::move(pipelineLayout))
                        .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
                        .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
                        .add_color_attachment(vk::Format::eB8G8R8A8Srgb)
                        .with_dynamic_viewport_scissor()
                        .build();

    EXPECT_NE(pipeline->handle(), nullptr);
}

TEST(GraphicsPipelineBuilderTest, PipelineWithFixedViewport) {
    auto &gpu = vw::tests::create_gpu();
    auto pipelineLayout = vw::PipelineLayoutBuilder(gpu.device).build();
    auto vertShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalVertexShaderSpirV);
    auto fragShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    auto pipeline =
        vw::GraphicsPipelineBuilder(gpu.device, std::move(pipelineLayout))
            .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
            .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
            .add_color_attachment(vk::Format::eB8G8R8A8Srgb)
            .with_fixed_viewport(800, 600)
            .with_fixed_scissor(800, 600)
            .build();

    EXPECT_NE(pipeline->handle(), nullptr);
}

TEST(GraphicsPipelineBuilderTest, PipelineWithDepthTest) {
    auto &gpu = vw::tests::create_gpu();
    auto pipelineLayout = vw::PipelineLayoutBuilder(gpu.device).build();
    auto vertShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalVertexShaderSpirV);
    auto fragShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    auto pipeline =
        vw::GraphicsPipelineBuilder(gpu.device, std::move(pipelineLayout))
            .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
            .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
            .add_color_attachment(vk::Format::eB8G8R8A8Srgb)
            .set_depth_format(vk::Format::eD32Sfloat)
            .with_depth_test(true, vk::CompareOp::eLess)
            .with_dynamic_viewport_scissor()
            .build();

    EXPECT_NE(pipeline->handle(), nullptr);
}

TEST(GraphicsPipelineBuilderTest, PipelineWithDifferentTopologies) {
    auto &gpu = vw::tests::create_gpu();
    auto pipelineLayout = vw::PipelineLayoutBuilder(gpu.device).build();
    auto vertShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalVertexShaderSpirV);
    auto fragShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    // Test triangle list (default)
    auto pipeline1 =
        vw::GraphicsPipelineBuilder(gpu.device, pipelineLayout)
            .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
            .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
            .add_color_attachment(vk::Format::eB8G8R8A8Srgb)
            .with_topology(vk::PrimitiveTopology::eTriangleList)
            .with_dynamic_viewport_scissor()
            .build();

    EXPECT_NE(pipeline1->handle(), nullptr);

    // Test line list
    auto pipeline2 =
        vw::GraphicsPipelineBuilder(gpu.device, pipelineLayout)
            .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
            .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
            .add_color_attachment(vk::Format::eB8G8R8A8Srgb)
            .with_topology(vk::PrimitiveTopology::eLineList)
            .with_dynamic_viewport_scissor()
            .build();

    EXPECT_NE(pipeline2->handle(), nullptr);
}

TEST(GraphicsPipelineBuilderTest, PipelineWithDifferentCullModes) {
    auto &gpu = vw::tests::create_gpu();
    auto pipelineLayout = vw::PipelineLayoutBuilder(gpu.device).build();
    auto vertShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalVertexShaderSpirV);
    auto fragShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    // Test no culling
    auto pipeline1 =
        vw::GraphicsPipelineBuilder(gpu.device, pipelineLayout)
            .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
            .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
            .add_color_attachment(vk::Format::eB8G8R8A8Srgb)
            .with_cull_mode(vk::CullModeFlagBits::eNone)
            .with_dynamic_viewport_scissor()
            .build();

    EXPECT_NE(pipeline1->handle(), nullptr);

    // Test front culling
    auto pipeline2 =
        vw::GraphicsPipelineBuilder(gpu.device, pipelineLayout)
            .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
            .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
            .add_color_attachment(vk::Format::eB8G8R8A8Srgb)
            .with_cull_mode(vk::CullModeFlagBits::eFront)
            .with_dynamic_viewport_scissor()
            .build();

    EXPECT_NE(pipeline2->handle(), nullptr);
}

TEST(GraphicsPipelineBuilderTest, PipelineWithMultipleColorAttachments) {
    auto &gpu = vw::tests::create_gpu();
    auto pipelineLayout = vw::PipelineLayoutBuilder(gpu.device).build();
    auto vertShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalVertexShaderSpirV);
    auto fragShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    auto pipeline =
        vw::GraphicsPipelineBuilder(gpu.device, std::move(pipelineLayout))
            .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
            .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
            .add_color_attachment(vk::Format::eR8G8B8A8Unorm)
            .add_color_attachment(vk::Format::eR16G16B16A16Sfloat)
            .add_color_attachment(vk::Format::eR32G32B32A32Sfloat)
            .with_dynamic_viewport_scissor()
            .build();

    EXPECT_NE(pipeline->handle(), nullptr);
}

TEST(GraphicsPipelineBuilderTest, PipelineWithVertexBinding) {
    auto &gpu = vw::tests::create_gpu();
    auto pipelineLayout = vw::PipelineLayoutBuilder(gpu.device).build();
    auto vertShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalVertexShaderSpirV);
    auto fragShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    auto pipeline =
        vw::GraphicsPipelineBuilder(gpu.device, std::move(pipelineLayout))
            .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
            .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
            .add_color_attachment(vk::Format::eB8G8R8A8Srgb)
            .add_vertex_binding<vw::ColoredVertex3D>()
            .with_dynamic_viewport_scissor()
            .build();

    EXPECT_NE(pipeline->handle(), nullptr);
}

TEST(GraphicsPipelineBuilderTest, PipelineLayoutAccessor) {
    auto &gpu = vw::tests::create_gpu();
    auto pipelineLayout = vw::PipelineLayoutBuilder(gpu.device).build();
    auto originalHandle = pipelineLayout.handle();
    auto vertShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalVertexShaderSpirV);
    auto fragShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    auto pipeline =
        vw::GraphicsPipelineBuilder(gpu.device, std::move(pipelineLayout))
            .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
            .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
            .add_color_attachment(vk::Format::eB8G8R8A8Srgb)
            .with_dynamic_viewport_scissor()
            .build();

    // The pipeline should have access to its layout
    EXPECT_EQ(pipeline->layout().handle(), originalHandle);
}

TEST(GraphicsPipelineBuilderTest, AddDynamicState) {
    auto &gpu = vw::tests::create_gpu();
    auto pipelineLayout = vw::PipelineLayoutBuilder(gpu.device).build();
    auto vertShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalVertexShaderSpirV);
    auto fragShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    auto pipeline =
        vw::GraphicsPipelineBuilder(gpu.device, std::move(pipelineLayout))
            .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
            .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
            .add_color_attachment(vk::Format::eB8G8R8A8Srgb)
            .add_dynamic_state(vk::DynamicState::eViewport)
            .add_dynamic_state(vk::DynamicState::eScissor)
            .add_dynamic_state(vk::DynamicState::eLineWidth)
            .build();

    EXPECT_NE(pipeline->handle(), nullptr);
}

TEST(GraphicsPipelineBuilderTest, DepthOnlyPipeline) {
    auto &gpu = vw::tests::create_gpu();
    auto pipelineLayout = vw::PipelineLayoutBuilder(gpu.device).build();
    auto vertShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalVertexShaderSpirV);
    auto fragShader = vw::ShaderModule::create_from_spirv(
        gpu.device, kMinimalFragmentShaderSpirV);

    auto pipeline =
        vw::GraphicsPipelineBuilder(gpu.device, std::move(pipelineLayout))
            .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
            .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
            .set_depth_format(vk::Format::eD32Sfloat)
            .with_depth_test(true, vk::CompareOp::eLess)
            .with_dynamic_viewport_scissor()
            .build();

    EXPECT_NE(pipeline->handle(), nullptr);
}

// Vertex Tests

TEST(VertexTest, ColoredVertex3DBindingDescription) {
    auto binding = vw::ColoredVertex3D::binding_description(0);
    EXPECT_EQ(binding.binding, 0);
    EXPECT_EQ(binding.stride, sizeof(vw::ColoredVertex3D));
    EXPECT_EQ(binding.inputRate, vk::VertexInputRate::eVertex);
}

TEST(VertexTest, ColoredVertex3DAttributeDescriptions) {
    auto attributes = vw::ColoredVertex3D::attribute_descriptions(0, 0);
    EXPECT_EQ(attributes.size(), 2); // position + color

    // Position attribute
    EXPECT_EQ(attributes[0].location, 0);
    EXPECT_EQ(attributes[0].binding, 0);
    EXPECT_EQ(attributes[0].format, vk::Format::eR32G32B32Sfloat);
    EXPECT_EQ(attributes[0].offset, 0);

    // Color attribute
    EXPECT_EQ(attributes[1].location, 1);
    EXPECT_EQ(attributes[1].binding, 0);
    EXPECT_EQ(attributes[1].format, vk::Format::eR32G32B32Sfloat);
    EXPECT_EQ(attributes[1].offset, sizeof(glm::vec3));
}

TEST(VertexTest, FullVertex3DBindingDescription) {
    auto binding = vw::FullVertex3D::binding_description(0);
    EXPECT_EQ(binding.binding, 0);
    EXPECT_EQ(binding.stride, sizeof(vw::FullVertex3D));
    EXPECT_EQ(binding.inputRate, vk::VertexInputRate::eVertex);
}

TEST(VertexTest, FullVertex3DAttributeDescriptions) {
    auto attributes = vw::FullVertex3D::attribute_descriptions(0, 0);
    EXPECT_EQ(attributes.size(),
              5); // position + normal + tangent + bitangent + uv

    // All attributes should have correct bindings and incremental locations
    for (size_t i = 0; i < attributes.size(); ++i) {
        EXPECT_EQ(attributes[i].location, i);
        EXPECT_EQ(attributes[i].binding, 0);
    }
}

TEST(VertexTest, Vertex3DSimple) {
    auto binding = vw::Vertex3D::binding_description(0);
    EXPECT_EQ(binding.stride, sizeof(glm::vec3));

    auto attributes = vw::Vertex3D::attribute_descriptions(0, 0);
    EXPECT_EQ(attributes.size(), 1);
    EXPECT_EQ(attributes[0].format, vk::Format::eR32G32B32Sfloat);
}

TEST(VertexTest, ColoredVertex2D) {
    auto binding = vw::ColoredVertex2D::binding_description(1);
    EXPECT_EQ(binding.binding, 1);
    EXPECT_EQ(binding.stride, sizeof(vw::ColoredVertex2D));

    auto attributes = vw::ColoredVertex2D::attribute_descriptions(1, 5);
    EXPECT_EQ(attributes.size(), 2);
    EXPECT_EQ(attributes[0].location, 5);
    EXPECT_EQ(attributes[0].binding, 1);
    EXPECT_EQ(attributes[0].format, vk::Format::eR32G32Sfloat); // vec2
    EXPECT_EQ(attributes[1].location, 6);
    EXPECT_EQ(attributes[1].format, vk::Format::eR32G32B32Sfloat); // vec3
}

TEST(VertexTest, VertexConcept) {
    // These should compile if the types satisfy the Vertex concept
    EXPECT_TRUE(vw::Vertex<vw::ColoredVertex2D>);
    EXPECT_TRUE(vw::Vertex<vw::ColoredVertex3D>);
    EXPECT_TRUE(vw::Vertex<vw::FullVertex3D>);
    EXPECT_TRUE(vw::Vertex<vw::Vertex3D>);
    EXPECT_TRUE(vw::Vertex<vw::ColoredAndTexturedVertex2D>);
    EXPECT_TRUE(vw::Vertex<vw::ColoredAndTexturedVertex3D>);
}
