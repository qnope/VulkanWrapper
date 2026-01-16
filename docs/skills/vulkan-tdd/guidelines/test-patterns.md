# Vulkan Test Patterns

## Core Patterns

### Pattern 1: GPU Resource Test

For testing allocation and basic properties:

```cpp
TEST(ResourceTest, CreateResourceWithProperties) {
    auto& gpu = vw::tests::create_gpu();

    // Create resource
    auto resource = create_resource(*gpu.allocator, params);

    // Verify properties
    EXPECT_EQ(resource.size(), expected_size);
    EXPECT_TRUE(resource.handle());
    EXPECT_EQ(resource.format(), expected_format);
}
```

### Pattern 2: Render-and-Verify

For testing rendering output:

```cpp
TEST_F(RenderTest, ProducesCorrectOutput) {
    constexpr uint32_t width = 64;
    constexpr uint32_t height = 64;

    // 1. Create output image
    auto output = allocator->create_image_2D(
        vw::Width{width}, vw::Height{height}, false,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment |
            vk::ImageUsageFlagBits::eTransferSrc);

    auto outputView = vw::ImageViewBuilder(device, output)
        .setImageType(vk::ImageViewType::e2D)
        .build();

    // 2. Create staging buffer for readback
    constexpr size_t bufferSize = width * height * 4;
    using StagingBuffer = vw::Buffer<std::byte, true, vw::StagingBufferUsage>;
    auto staging = vw::create_buffer<StagingBuffer>(*allocator, bufferSize);

    // 3. Record commands
    auto cmd = cmdPool->allocate(1)[0];
    std::ignore = cmd.begin(vk::CommandBufferBeginInfo().setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    vw::Transfer transfer;
    auto& tracker = transfer.resourceTracker();

    // Transition to render target
    tracker.request(vw::Barrier::ImageState{
        .image = output->handle(),
        .subresourceRange = outputView->subresource_range(),
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite
    });
    tracker.flush(cmd);

    // 4. Render
    render_something(cmd, outputView);

    // 5. Copy to staging
    transfer.copyImageToBuffer(cmd, output, staging.handle(), 0);

    std::ignore = cmd.end();

    // 6. Submit and wait
    queue->enqueue_command_buffer(cmd);
    queue->submit({}, {}, {}).wait();

    // 7. Verify pixels
    auto pixels = staging.read_as_vector(0, bufferSize);
    EXPECT_EQ(static_cast<uint8_t>(pixels[0]), expected_r);
    EXPECT_EQ(static_cast<uint8_t>(pixels[1]), expected_g);
    EXPECT_EQ(static_cast<uint8_t>(pixels[2]), expected_b);
    EXPECT_EQ(static_cast<uint8_t>(pixels[3]), expected_a);
}
```

### Pattern 3: Shader Compilation Test

For testing shader compilation:

```cpp
const std::string TEST_SHADER = R"(
#version 450

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

TEST(ShaderTest, CompilesValidShader) {
    auto& gpu = vw::tests::create_gpu();
    vw::ShaderCompiler compiler;

    auto module = compiler.compile_to_module(
        gpu.device, TEST_SHADER, vk::ShaderStageFlagBits::eFragment);

    ASSERT_NE(module, nullptr);
    EXPECT_TRUE(module->handle());
}

TEST(ShaderTest, ThrowsOnInvalidShader) {
    auto& gpu = vw::tests::create_gpu();
    vw::ShaderCompiler compiler;

    EXPECT_THROW(
        compiler.compile_to_module(gpu.device, "invalid glsl",
                                   vk::ShaderStageFlagBits::eFragment),
        vw::ShaderCompilationException);
}
```

### Pattern 4: Buffer Read/Write Test

For testing buffer operations:

```cpp
TEST(BufferTest, WriteAndReadData) {
    auto& gpu = vw::tests::create_gpu();

    using HostBuffer = vw::Buffer<float, true, vw::UniformBufferUsage>;
    auto buffer = vw::create_buffer<HostBuffer>(*gpu.allocator, 100);

    // Write data
    std::vector<float> writeData = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    buffer.write(std::span<const float>(writeData), 0);

    // Read back
    auto readData = buffer.read_as_vector(0, writeData.size());

    // Verify
    ASSERT_EQ(readData.size(), writeData.size());
    for (size_t i = 0; i < writeData.size(); ++i) {
        EXPECT_FLOAT_EQ(readData[i], writeData[i]);
    }
}
```

### Pattern 5: Descriptor Set Test

For testing descriptor binding:

```cpp
TEST_F(DescriptorTest, BindsCombinedImageSampler) {
    // Create resources
    auto image = allocator->create_image_2D(...);
    auto view = vw::ImageViewBuilder(device, image).build();
    auto sampler = vw::SamplerBuilder(device).build();

    // Create descriptor layout
    auto layout = vw::DescriptorSetLayoutBuilder(device)
        .with_combined_image(vk::ShaderStageFlagBits::eFragment, 1)
        .build();

    // Create pool and allocate set
    auto pool = vw::DescriptorPoolBuilder(device, layout).build();
    vw::DescriptorAllocator allocator;
    allocator.add_combined_image(0, vw::CombinedImage(view, sampler),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead);

    auto set = pool.allocate_set(allocator);

    EXPECT_TRUE(set.handle());
    EXPECT_FALSE(set.resources().empty());
}
```

### Pattern 6: Pipeline Creation Test

For testing pipeline setup:

```cpp
TEST(PipelineTest, CreatesGraphicsPipeline) {
    auto& gpu = vw::tests::create_gpu();
    vw::ShaderCompiler compiler;

    auto vertShader = compiler.compile_to_module(
        gpu.device, VERTEX_SHADER, vk::ShaderStageFlagBits::eVertex);
    auto fragShader = compiler.compile_to_module(
        gpu.device, FRAGMENT_SHADER, vk::ShaderStageFlagBits::eFragment);

    auto layout = vw::DescriptorSetLayoutBuilder(gpu.device).build();

    auto pipeline = vw::create_screen_space_pipeline(
        gpu.device, vertShader, fragShader, layout,
        vk::Format::eR8G8B8A8Unorm);

    ASSERT_NE(pipeline, nullptr);
    EXPECT_TRUE(pipeline->handle());
}
```

## Advanced Patterns

### Pattern 7: Multi-Frame Test

For testing frame-to-frame consistency:

```cpp
TEST_F(MultiFrameTest, MaintainsStateAcrossFrames) {
    constexpr size_t frameCount = 3;

    for (size_t frame = 0; frame < frameCount; ++frame) {
        auto cmd = cmdPool->allocate(1)[0];
        // Record frame commands
        record_frame(cmd, frame);

        queue->enqueue_command_buffer(cmd);
        queue->submit({}, {}, {}).wait();

        // Verify frame output
        auto pixels = readback_frame(frame);
        verify_frame_output(pixels, frame);
    }
}
```

### Pattern 8: Ray Tracing Test

For testing ray tracing features:

```cpp
TEST_F(RayTracingTest, BuildsAccelerationStructure) {
    // Create geometry
    auto vertices = create_triangle_vertices();
    auto indices = create_triangle_indices();

    // Build BLAS
    auto blas = vw::rt::BottomLevelAccelerationStructure(
        device, allocator, vertices, indices);

    // Build TLAS
    std::vector<vw::rt::Instance> instances = {{
        .blas = &blas,
        .transform = glm::mat4(1.0f)
    }};
    auto tlas = vw::rt::TopLevelAccelerationStructure(
        device, allocator, instances);

    EXPECT_TRUE(blas.handle());
    EXPECT_TRUE(tlas.handle());
}
```

### Pattern 9: Resource State Tracking Test

For testing barrier generation:

```cpp
TEST(ResourceTrackerTest, GeneratesCorrectBarriers) {
    vw::ResourceTracker tracker;

    // Track initial state
    tracker.track(vw::Barrier::ImageState{
        .image = image,
        .layout = vk::ImageLayout::eUndefined,
        .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
        .access = vk::AccessFlagBits2::eNone
    });

    // Request new state
    tracker.request(vw::Barrier::ImageState{
        .image = image,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
        .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .access = vk::AccessFlagBits2::eColorAttachmentWrite
    });

    // Flush should generate barrier
    tracker.flush(cmd);

    // Verify state was updated
    auto state = tracker.get_state(image);
    EXPECT_EQ(state.layout, vk::ImageLayout::eColorAttachmentOptimal);
}
```

## Pixel Verification Utilities

### Exact Match
```cpp
void expect_pixel_exact(std::span<const std::byte> pixels, size_t index,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    size_t i = index * 4;
    EXPECT_EQ(static_cast<uint8_t>(pixels[i + 0]), r) << "R at " << index;
    EXPECT_EQ(static_cast<uint8_t>(pixels[i + 1]), g) << "G at " << index;
    EXPECT_EQ(static_cast<uint8_t>(pixels[i + 2]), b) << "B at " << index;
    EXPECT_EQ(static_cast<uint8_t>(pixels[i + 3]), a) << "A at " << index;
}
```

### Approximate Match (for gradients)
```cpp
void expect_pixel_near(std::span<const std::byte> pixels, size_t index,
                       uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                       int tolerance = 5) {
    size_t i = index * 4;
    EXPECT_NEAR(static_cast<uint8_t>(pixels[i + 0]), r, tolerance);
    EXPECT_NEAR(static_cast<uint8_t>(pixels[i + 1]), g, tolerance);
    EXPECT_NEAR(static_cast<uint8_t>(pixels[i + 2]), b, tolerance);
    EXPECT_NEAR(static_cast<uint8_t>(pixels[i + 3]), a, tolerance);
}
```

### Full Image Comparison
```cpp
void expect_solid_color(std::span<const std::byte> pixels,
                        uint32_t width, uint32_t height,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    for (size_t i = 0; i < width * height; ++i) {
        expect_pixel_exact(pixels, i, r, g, b, a);
    }
}
```

## Test Data Helpers

### Common Shader Sources

```cpp
namespace TestShaders {

const std::string FULLSCREEN_VERT = R"(
#version 450
layout(location = 0) out vec2 fragUV;
void main() {
    vec2 positions[4] = vec2[](
        vec2(-1.0, -1.0), vec2(1.0, -1.0),
        vec2(-1.0, 1.0), vec2(1.0, 1.0)
    );
    vec2 uvs[4] = vec2[](
        vec2(0.0, 0.0), vec2(1.0, 0.0),
        vec2(0.0, 1.0), vec2(1.0, 1.0)
    );
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragUV = uvs[gl_VertexIndex];
}
)";

const std::string SOLID_RED_FRAG = R"(
#version 450
layout(location = 0) out vec4 outColor;
void main() { outColor = vec4(1.0, 0.0, 0.0, 1.0); }
)";

const std::string UV_GRADIENT_FRAG = R"(
#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;
void main() { outColor = vec4(fragUV, 0.0, 1.0); }
)";

} // namespace TestShaders
```

### Common Test Sizes

```cpp
namespace TestSizes {
    constexpr uint32_t SMALL = 16;    // Quick tests
    constexpr uint32_t MEDIUM = 64;   // Standard tests
    constexpr uint32_t LARGE = 256;   // Stress tests
}
```
