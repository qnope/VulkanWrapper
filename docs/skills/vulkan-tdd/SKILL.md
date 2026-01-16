# Vulkan TDD Skill

This skill provides Test-Driven Development guidance for VulkanWrapper development.

## Invocation

Use `/vulkan-tdd` to activate this skill when:
- Implementing new Vulkan features
- Adding new render passes or pipelines
- Fixing bugs in graphics code
- Refactoring Vulkan-related code

## Workflow Overview

The TDD workflow follows the **Red-Green-Refactor** cycle:

1. **RED**: Write a failing test that defines expected behavior
2. **GREEN**: Write minimal code to make the test pass
3. **REFACTOR**: Improve code quality while keeping tests green

## Quick Reference

### Run Tests
```bash
cd build-Clang20Debug && ctest --verbose
```

### Run Specific Test
```bash
cd build-Clang20Debug/VulkanWrapper/tests
./MemoryTests --gtest_filter="BufferTest.*"
```

### Build After Changes
```bash
cmake --build build-Clang20Debug --target <TestTarget>
```

## Guidelines

Read these files for detailed guidance:

- [TDD Workflow](guidelines/tdd-workflow.md) - Step-by-step TDD process
- [Test Patterns](guidelines/test-patterns.md) - Vulkan-specific test patterns
- [Review Checklist](guidelines/review-checklist.md) - Code review criteria

## Templates

Use these templates as starting points:

- [Basic Test Template](templates/test-template.cpp)
- [Render Pass Test Template](templates/render-pass-test-template.cpp)
- [Buffer Test Template](templates/buffer-test-template.cpp)

## Key Principles

### 1. Test Isolation
Each test must be independent and not rely on state from other tests.
Use the `vw::tests::create_gpu()` singleton for GPU resources.

### 2. Deterministic Results
- Use software rendering (llvmpipe) for consistent results
- Set fixed seeds for any randomness
- Verify pixel-exact outputs when possible

### 3. Resource Cleanup
- RAII handles cleanup automatically
- `device->wait_idle()` before test teardown if needed
- Let shared_ptr manage lifetimes

### 4. Error Validation
- Test both success and failure paths
- Verify exception messages contain useful info
- Check Vulkan validation layer output

## Test Categories

| Category | Test File | Description |
|----------|-----------|-------------|
| Memory | `BufferTests.cpp` | Buffer allocation, read/write |
| Images | `ImageTests.cpp` | Image creation, formats |
| Render | `ScreenSpacePassTests.cpp` | Full rendering verification |
| RT | `RayTracedSceneTests.cpp` | Ray tracing features |
| Shaders | `ShaderCompilerTests.cpp` | Shader compilation |

## API Reference

### Test GPU Singleton
```cpp
auto& gpu = vw::tests::create_gpu();
// gpu.device - Vulkan device
// gpu.allocator - VMA allocator
// gpu.queue() - Graphics queue
```

### Common Test Setup
```cpp
class MyFeatureTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& gpu = vw::tests::create_gpu();
        device = gpu.device;
        allocator = gpu.allocator;
        queue = &gpu.queue();
        cmdPool = std::make_unique<vw::CommandPool>(
            vw::CommandPoolBuilder(device).build());
    }

    std::shared_ptr<vw::Device> device;
    std::shared_ptr<vw::Allocator> allocator;
    vw::Queue* queue;
    std::unique_ptr<vw::CommandPool> cmdPool;
};
```

### Pixel Verification Pattern
```cpp
// Create staging buffer for readback
auto staging = vw::create_buffer<StagingBuffer>(*allocator, size);

// Copy image to buffer
transfer.copyImageToBuffer(cmd, image, staging.handle(), 0);

// Submit and wait
queue->enqueue_command_buffer(cmd);
queue->submit({}, {}, {}).wait();

// Verify pixels
auto pixels = staging.read_as_vector(0, size);
EXPECT_EQ(static_cast<uint8_t>(pixels[0]), expectedR);
```
