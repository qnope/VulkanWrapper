/**
 * Tutorial 01: Hello Triangle
 *
 * This tutorial demonstrates the minimal code needed to render a colored
 * triangle using VulkanWrapper. It covers:
 *
 * 1. Setting up a graphics pipeline
 * 2. Creating vertex buffers
 * 3. Recording draw commands
 * 4. Producing a screenshot
 *
 * Expected output: A colored triangle on a dark background
 */

#include "TutorialFramework/TutorialApp.h"

#include <VulkanWrapper/Descriptors/Vertex.h>
#include <VulkanWrapper/Memory/Buffer.h>
#include <VulkanWrapper/Pipeline/GraphicsPipelineBuilder.h>
#include <VulkanWrapper/Pipeline/PipelineLayoutBuilder.h>
#include <VulkanWrapper/Shader/ShaderCompiler.h>

#include <array>

using namespace vw;
using namespace vw::tutorial;

// Vertex structure with position and color
struct TriangleVertex {
    glm::vec3 position;
    glm::vec3 color;

    static constexpr auto bindings() {
        return std::array{vk::VertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(TriangleVertex),
            .inputRate = vk::VertexInputRate::eVertex}};
    }

    static constexpr auto attributes() {
        return std::array{
            vk::VertexInputAttributeDescription{.location = 0,
                                                .binding = 0,
                                                .format =
                                                    vk::Format::eR32G32B32Sfloat,
                                                .offset = offsetof(
                                                    TriangleVertex, position)},
            vk::VertexInputAttributeDescription{
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(TriangleVertex, color)}};
    }
};

// Triangle vertices: position (x, y, z), color (r, g, b)
constexpr std::array<TriangleVertex, 3> triangleVertices = {{
    {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},   // Top vertex (red)
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},   // Bottom left (green)
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}     // Bottom right (blue)
}};

// Vertex shader source (GLSL)
constexpr const char *vertexShaderSource = R"(
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
}
)";

// Fragment shader source (GLSL)
constexpr const char *fragmentShaderSource = R"(
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
)";

class HelloTriangle : public TutorialApp {
public:
    HelloTriangle()
        : TutorialApp(TutorialConfig{
              .name = "Hello Triangle",
              .width = 800,
              .height = 600,
              .frameCount = 1,
              .screenshotPath = "screenshot.png"}) {}

protected:
    void setup() override {
        // Compile shaders at runtime
        ShaderCompiler compiler;

        auto vertexSpirv = compiler.compile(
            vertexShaderSource, shaderc_vertex_shader, "triangle.vert");
        auto fragmentSpirv = compiler.compile(
            fragmentShaderSource, shaderc_fragment_shader, "triangle.frag");

        m_vertexShader =
            std::make_shared<ShaderModule>(device(), std::move(vertexSpirv));
        m_fragmentShader =
            std::make_shared<ShaderModule>(device(), std::move(fragmentSpirv));

        // Create pipeline layout (no descriptors or push constants)
        m_pipelineLayout = PipelineLayoutBuilder(device()).build();

        // Create graphics pipeline
        m_pipeline =
            GraphicsPipelineBuilder(device())
                .set_layout(m_pipelineLayout)
                .add_shader(vk::ShaderStageFlagBits::eVertex, m_vertexShader)
                .add_shader(vk::ShaderStageFlagBits::eFragment, m_fragmentShader)
                .set_vertex_input<TriangleVertex>()
                .set_topology(vk::PrimitiveTopology::eTriangleList)
                .set_polygon_mode(vk::PolygonMode::eFill)
                .set_cull_mode(vk::CullModeFlagBits::eNone)
                .set_front_face(vk::FrontFace::eCounterClockwise)
                .add_dynamic_state(vk::DynamicState::eViewport)
                .add_dynamic_state(vk::DynamicState::eScissor)
                .add_color_attachment(config().colorFormat)
                .build();

        // Create vertex buffer
        m_vertexBuffer = allocator()->create_buffer<TriangleVertex, true,
                                                    VertexBufferUsage>(
            triangleVertices.size());

        // Upload vertex data
        auto span = m_vertexBuffer->data();
        std::ranges::copy(triangleVertices, span.begin());
    }

    void render(vk::CommandBuffer cmd, uint32_t /*frameIndex*/) override {
        // Begin rendering to render target
        beginRendering(cmd, vk::AttachmentLoadOp::eClear,
                       {{{0.1f, 0.1f, 0.15f, 1.0f}}});

        // Bind pipeline
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,
                         m_pipeline->handle());

        // Bind vertex buffer
        cmd.bindVertexBuffers(0, m_vertexBuffer->handle(), vk::DeviceSize{0});

        // Draw triangle
        cmd.draw(3, 1, 0, 0);

        // End rendering
        endRendering(cmd);
    }

    void cleanup() override {
        m_vertexBuffer.reset();
        m_pipeline.reset();
        m_pipelineLayout.reset();
        m_fragmentShader.reset();
        m_vertexShader.reset();
    }

private:
    std::shared_ptr<ShaderModule> m_vertexShader;
    std::shared_ptr<ShaderModule> m_fragmentShader;
    std::shared_ptr<PipelineLayout> m_pipelineLayout;
    std::shared_ptr<Pipeline> m_pipeline;
    std::shared_ptr<Buffer<TriangleVertex, true, VertexBufferUsage>>
        m_vertexBuffer;
};

int main() {
    try {
        HelloTriangle app;
        app.run();
        return 0;
    } catch (const vw::Exception &e) {
        std::cerr << "VulkanWrapper Error: " << e.what() << std::endl;
        std::cerr << "  at " << e.location().file_name() << ":"
                  << e.location().line() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
