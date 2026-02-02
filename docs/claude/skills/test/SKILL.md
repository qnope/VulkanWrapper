# Test Skill

GTest-based testing for VulkanWrapper.

## Quick Start

```cpp
#include "utils/create_gpu.hpp"
#include <gtest/gtest.h>

TEST(MyFeature, BasicTest) {
    auto& gpu = vw::tests::create_gpu();
    // gpu.device, gpu.allocator, gpu.queue()
    EXPECT_TRUE(gpu.device->handle());
}
```

## Build & Run

```bash
cmake --build build-Clang20Debug --target RenderPassTests
cd build-Clang20Debug/VulkanWrapper/tests && ./RenderPassTests

# Filter tests
./RenderPassTests --gtest_filter='*IndirectLight*'
```

## Test Types

| Type | File | Purpose |
|------|------|---------|
| Unit | [UnitTest.md](UnitTest.md) | Logic, APIs, memory |
| Rendering | [RenderingTest.md](RenderingTest.md) | Visual coherence (relationships, not pixels) |

## Key Principles

1. **Use GPU singleton:** `vw::tests::create_gpu()`
2. **Skip unavailable features:** `GTEST_SKIP() << "Ray tracing unavailable"`
3. **Wait for GPU:** `gpu.queue().submit({}, {}, {}).wait()`
4. **Test relationships, not pixels:** "lit > shadowed", not "pixel == RGB(127,89,45)"

## Test Executables

`MemoryTests`, `ImageTests`, `VulkanTests`, `RenderPassTests`, `MaterialTests`, `ShaderTests`, `RayTracingTests`, `UtilsTests`, `RandomTests`
