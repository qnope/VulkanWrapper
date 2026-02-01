# Test Agent Skill

This skill provides guidance for writing and running tests in the VulkanWrapper project. Tests are organized into two main categories:

- **Unit Tests**: Standard GTest-based tests for validating logic, APIs, and functionality
- **Rendering Tests**: GPU-based tests that validate visual output through coherence checks

## Quick Start

Minimal test example to get started:

```cpp
// VulkanWrapper/tests/Example/MyTest.cpp
#include "utils/create_gpu.hpp"
#include <gtest/gtest.h>

namespace vw::tests {

TEST(MyFeature, BasicTest) {
    auto& gpu = create_gpu();

    // Your test using gpu.device, gpu.allocator, gpu.queue()
    EXPECT_TRUE(gpu.device->handle());
}

} // namespace vw::tests
```

Build and run:
```bash
cmake --build build-Clang20Debug --target MyTests
cd build-Clang20Debug/VulkanWrapper/tests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./MyTests
```

## Quick Reference

| Category | Documentation | Purpose |
|----------|--------------|---------|
| Unit Tests | [UnitTest.md](./UnitTest.md) | Test logic, APIs, memory management |
| Rendering Tests | [RenderingTest.md](./RenderingTest.md) | Validate visual coherence (lighting, shadows, colors) |

## Test Infrastructure Overview

### Directory Structure

```
VulkanWrapper/tests/
├── utils/                    # Shared test utilities
│   ├── create_gpu.hpp        # GPU singleton for tests
│   └── create_gpu.cpp
├── Memory/                   # Memory allocation tests
├── Image/                    # Image handling tests
├── Vulkan/                   # Core Vulkan tests
├── RenderPass/               # Rendering pass tests
├── Material/                 # Material system tests
├── Shader/                   # Shader compilation tests
├── RayTracing/               # Ray tracing tests
└── Random/                   # Random sampling tests
```

### Test Executables

Tests are organized into separate executables:
- `MemoryTests`
- `ImageTests`
- `VulkanTests`
- `RenderPassTests`
- `MaterialTests`
- `ShaderTests`
- `RayTracingTests`
- `UtilsTests`
- `RandomTests`

### Building Tests

Tests must be built before running:

```bash
# Build all tests
cmake --build build-Clang20Debug

# Build specific test executable
cmake --build build-Clang20Debug --target RenderPassTests
```

### Running Tests

```bash
# Run all tests
cd build-Clang20Debug && DYLD_LIBRARY_PATH=vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ctest

# Run specific test executable
cd build-Clang20Debug/VulkanWrapper/tests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./RenderPassTests

# Run with verbose output
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ctest --verbose

# Filter specific tests
DYLD_LIBRARY_PATH=../../vcpkg_installed/arm64-osx/lib:$DYLD_LIBRARY_PATH ./RenderPassTests --gtest_filter='*IndirectLight*'
```

## Key Principles

1. **Functional Correctness Over Visual Regression**
   - Tests validate numerical correctness, not pixel-perfect images
   - No golden image files or screenshot comparisons
   - CPU-side reference implementations for comparison

2. **Coherence Testing for Rendering**
   - Validate relationships (lit vs shadowed, warm vs cool)
   - Test physical plausibility (Rayleigh scattering, attenuation)
   - Allow tolerance for stochastic rendering methods

3. **GPU Singleton Pattern**
   - Use `vw::tests::create_gpu()` for standard GPU access
   - Use specialized GPU structs for ray tracing tests
   - Intentionally leaked to avoid static destruction order issues

## When to Use Each Test Type

| Scenario | Test Type | Example |
|----------|-----------|---------|
| Buffer allocation | Unit Test | Verify buffer size, alignment |
| Pipeline creation | Unit Test | Validate descriptor layouts |
| Shader compilation | Unit Test | Check SPIR-V generation |
| Shadow visibility | Rendering Test | Lit surface brighter than shadowed |
| Sky color at sunset | Rendering Test | More red than blue |
| Tone mapping | Rendering Test | Match CPU reference within tolerance |

## What NOT to Test

- **Vulkan driver behavior**: Don't test that `vkCreateBuffer` works; trust the driver
- **Third-party libraries**: Don't test GLM math or VMA allocation internals
- **Obvious pass-through**: If your function just calls Vulkan, test the higher-level behavior instead

Focus on testing **your logic**: state management, resource tracking, barrier generation, rendering correctness.

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "Ray tracing not available" | Normal on some systems; test uses `GTEST_SKIP()` |
| Validation layer errors in stderr | Check for missing barriers, invalid handles, or sync issues |
| Test hangs | GPU deadlock; add `device->wait_idle()` or check fence/semaphore usage |
| Flaky stochastic tests | Increase accumulation frames or relax tolerance |
| `DYLD_LIBRARY_PATH` errors | Ensure path points to `vcpkg_installed/arm64-osx/lib` |

## CI Considerations

- **llvmpipe** (software Vulkan) provides deterministic results, ideal for CI
- Tests should complete in reasonable time; avoid excessive accumulation frames
- Ray tracing tests work on llvmpipe (full RT support)

## See Also

- [UnitTest.md](./UnitTest.md) - Detailed unit test patterns and examples
- [RenderingTest.md](./RenderingTest.md) - Rendering coherence test patterns
