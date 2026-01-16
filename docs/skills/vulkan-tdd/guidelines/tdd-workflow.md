# TDD Workflow for Vulkan Development

## The Red-Green-Refactor Cycle

### Phase 1: RED - Write a Failing Test

Before writing any implementation code:

1. **Understand the requirement**
   - What behavior needs to be implemented?
   - What are the inputs and expected outputs?
   - What edge cases exist?

2. **Write the test first**
   ```cpp
   TEST(FeatureTest, DescriptiveBehaviorName) {
       // Arrange - Set up test data
       auto& gpu = vw::tests::create_gpu();

       // Act - Call the code under test
       auto result = feature_under_test(...);

       // Assert - Verify expected behavior
       EXPECT_EQ(result, expected_value);
   }
   ```

3. **Verify the test fails**
   - Compile the test (expect compilation errors for missing code)
   - Add minimal stubs to make it compile
   - Run the test and confirm it fails with expected error

4. **Good test names describe behavior**
   - `CreateBufferWithValidSize` not `TestBuffer1`
   - `ThrowsOnInvalidFormat` not `ErrorTest`
   - `ReturnsEmptyWhenNoData` not `EmptyTest`

### Phase 2: GREEN - Make the Test Pass

Write the minimum code to pass the test:

1. **Focus on making the test pass**
   - Don't optimize prematurely
   - Don't add features the test doesn't require
   - Don't refactor yet

2. **Implementation guidelines**
   ```cpp
   // Start with the simplest implementation
   Buffer create_buffer(size_t size) {
       // Just enough to pass the test
       return allocator->create_buffer(size);
   }
   ```

3. **Run tests frequently**
   ```bash
   cmake --build build-Clang20Debug --target MyTests && \
   cd build-Clang20Debug/VulkanWrapper/tests && ./MyTests
   ```

4. **One test at a time**
   - Get one test passing before writing the next
   - Build up functionality incrementally

### Phase 3: REFACTOR - Improve the Code

Once tests pass, improve code quality:

1. **What to refactor**
   - Remove duplication
   - Improve naming
   - Extract helper functions
   - Simplify complex logic

2. **Keep tests green**
   - Run tests after each refactor
   - Never refactor and add features simultaneously

3. **Vulkan-specific refactoring**
   - Extract barrier generation to ResourceTracker
   - Consolidate descriptor set updates
   - Reuse shader modules
   - Optimize memory allocations

## TDD for Vulkan Features

### New Buffer Type

```cpp
// 1. RED: Write test for new buffer type
TEST(BufferTest, CreateCustomBuffer) {
    auto& gpu = vw::tests::create_gpu();
    using CustomBuffer = vw::Buffer<MyStruct, true, CustomUsage>;
    auto buffer = vw::create_buffer<CustomBuffer>(*gpu.allocator, 100);

    EXPECT_EQ(buffer.size(), 100);
}

// 2. GREEN: Add buffer usage flags and type alias
// 3. REFACTOR: Ensure consistency with other buffer types
```

### New Render Pass

```cpp
// 1. RED: Write test for render pass output
TEST_F(RenderPassTest, ProducesExpectedOutput) {
    // Create pass and render
    MyPass pass(device, allocator);
    pass.execute(cmd, input, output);

    // Verify output pixels
    auto pixels = readback(output);
    EXPECT_EQ(pixels[0], expected_color);
}

// 2. GREEN: Implement minimal pass that produces correct output
// 3. REFACTOR: Extract common patterns, optimize barriers
```

### New Shader Feature

```cpp
// 1. RED: Write test that uses new shader feature
TEST(ShaderTest, SupportsNewFeature) {
    vw::ShaderCompiler compiler;
    auto shader = compiler.compile_to_module(device, SHADER_WITH_FEATURE, stage);
    ASSERT_NE(shader, nullptr);
}

// 2. GREEN: Ensure shader compiles and runs
// 3. REFACTOR: Add to shader library if reusable
```

## Test Organization

### Test File Structure

```
VulkanWrapper/tests/
├── Module/
│   ├── FeatureTests.cpp      # Feature-specific tests
│   └── IntegrationTests.cpp  # Cross-feature tests
├── utils/
│   ├── create_gpu.hpp        # Test GPU singleton
│   └── test_helpers.hpp      # Common utilities
└── CMakeLists.txt            # Test configuration
```

### Test Naming Convention

```cpp
// Test fixture for a class/module
class BufferTest : public ::testing::Test { ... };

// Test cases describe specific behavior
TEST_F(BufferTest, AllocatesCorrectSize) { ... }
TEST_F(BufferTest, SupportsHostVisibleMemory) { ... }
TEST_F(BufferTest, ThrowsOnZeroSize) { ... }
```

## Debugging Failed Tests

### When a Test Fails

1. **Read the error message carefully**
   ```
   Expected: 255
   Actual: 0
   ```

2. **Check validation layers**
   ```bash
   VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./MyTests
   ```

3. **Add diagnostic output**
   ```cpp
   // Temporarily add for debugging
   std::cout << "Buffer size: " << buffer.size() << std::endl;
   ```

4. **Use LLDB for complex issues**
   ```bash
   lldb ./MyTests -- --gtest_filter="FailingTest"
   (lldb) breakpoint set --name test_function
   (lldb) run
   ```

### Common Failure Patterns

| Symptom | Likely Cause |
|---------|--------------|
| Black pixels | Missing barrier transition |
| Validation error | Invalid resource state |
| Crash on submit | Uninitialized descriptor |
| Timeout | GPU hang from infinite loop |

## Incremental Development

### Building Features Incrementally

```
Feature: Add bloom post-processing pass

Tests to write (in order):
1. BloomPassCreatesOutputImage
2. BloomPassDownsamples
3. BloomPassBlurs
4. BloomPassUpsamples
5. BloomPassCombinesWithOriginal
6. BloomPassHandlesVariableIntensity
```

Each test builds on the previous, gradually expanding functionality.

### Commit Strategy

```
git add -A && git commit -m "test: add bloom pass output test (RED)"
# Implement minimal code
git add -A && git commit -m "feat: bloom pass creates output image (GREEN)"
# Refactor
git add -A && git commit -m "refactor: extract bloom sampling helper"
# Next test
git add -A && git commit -m "test: add bloom downsampling test (RED)"
```
