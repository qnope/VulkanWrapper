# Testing Skill for VulkanWrapper

This skill provides comprehensive guidance for writing and maintaining tests in the VulkanWrapper project. Tests are essential for ensuring correctness and preventing regressions in this Vulkan graphics library.

## Overview

The VulkanWrapper test suite uses **Google Test (GTest)** as the testing framework and is organized into two main categories:

1. **[Unit Tests](./UnitTest.md)** - Pure logic tests that verify algorithms, data structures, and CPU-side computations
2. **[Rendering Tests](./RenderingTest.md)** - GPU-based tests that validate visual output through coherence checks

## Key Principles

### Coherence Over Pixel-Perfection

The most important principle for rendering tests is **coherence validation**, not pixel-perfect matching:

- A shadowed area should be **darker** than a sunlit area
- Sunset sky colors should be **warmer (more red)** than noon sky colors
- A surface facing up should receive **more sky light** than a surface facing down

This approach is more robust than exact pixel comparisons because:
- GPU floating-point precision varies between vendors
- Stochastic sampling (ray tracing) introduces variance
- Minor shader optimizations shouldn't break tests

### Test Categories

| Category | Purpose | GPU Required |
|----------|---------|--------------|
| Unit Tests | Algorithm correctness | No (some tests) |
| Memory Tests | Buffer/allocation behavior | Yes |
| Image Tests | Image creation/manipulation | Yes |
| Rendering Tests | Visual output validation | Yes |
| Ray Tracing Tests | RT-specific features | Yes (RT capable) |

## Quick Start

### Running All Tests

```bash
cd build-Clang20Debug && ctest --verbose
```

### Running Specific Test Executables

```bash
cd build-Clang20Debug/VulkanWrapper/tests

# Unit tests (faster)
./MemoryTests
./UtilsTests

# Rendering tests (require GPU)
./RenderPassTests
./RayTracingTests
```

## Documentation Structure

- **[UnitTest.md](./UnitTest.md)** - Detailed guide for writing unit tests
  - Test fixture patterns
  - GPU singleton usage
  - Buffer read/write testing
  - Algorithm verification

- **[RenderingTest.md](./RenderingTest.md)** - Comprehensive guide for rendering tests
  - Coherence validation philosophy
  - Helper functions for pixel readback
  - G-buffer setup patterns
  - Atmospheric/lighting coherence examples

## Test File Organization

```
VulkanWrapper/tests/
├── CMakeLists.txt           # Test configuration
├── utils/
│   ├── create_gpu.hpp       # GPU singleton header
│   └── create_gpu.cpp       # GPU singleton implementation
├── Memory/                  # Buffer, allocator tests
├── Image/                   # Image manipulation tests
├── RenderPass/              # Rendering pass tests
├── RayTracing/              # Ray tracing tests
├── Material/                # Material system tests
├── Shader/                  # Shader compilation tests
└── Random/                  # Random sampling tests
```

## Adding New Tests

1. Create test file in the appropriate subdirectory
2. Add to `VulkanWrapper/tests/CMakeLists.txt`
3. Link required libraries:
   - `TestUtils` (provides `create_gpu()`)
   - `VulkanWrapperCoreLibrary`
   - `GTest::gtest`
   - `GTest::gtest_main`
4. Call `gtest_discover_tests(YourTestExecutable)`

## See Also

- [UnitTest.md](./UnitTest.md) - Unit testing documentation
- [RenderingTest.md](./RenderingTest.md) - Rendering testing documentation
- [CLAUDE.md](../../CLAUDE.md) - Project overview and build instructions
