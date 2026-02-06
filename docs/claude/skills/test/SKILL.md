# Test Skill

GTest-based testing for VulkanWrapper. Tests run on llvmpipe (software Vulkan with full RT support).

## Quick Start

```cpp
#include "utils/create_gpu.hpp"
#include <gtest/gtest.h>

TEST(MyFeature, BasicTest) {
    auto& gpu = vw::tests::create_gpu();
    // gpu.instance, gpu.device, gpu.allocator, gpu.queue()
    EXPECT_TRUE(gpu.device->handle());
}
```

## Test Types

| Type | File | Purpose |
|------|------|---------|
| Unit | [UnitTest.md](UnitTest.md) | GPU resources, memory, APIs, exceptions |
| Rendering | [RenderingTest.md](RenderingTest.md) | Visual coherence (relationships, not pixels) |

## Key Principles

1. **Use GPU singleton:** `vw::tests::create_gpu()` (defined in `tests/utils/create_gpu.hpp`)
2. **Skip unavailable features:** `GTEST_SKIP() << "Ray tracing unavailable"`
3. **Always include `<< "explanation"` in assertions**
4. **Wait for GPU after submit:** `gpu.queue().submit({}, {}, {}).wait()`
5. **Test relationships, not pixels:** "lit > shadowed", not "pixel == RGB(127,89,45)"
6. **Validation layers always on:** The test GPU singleton enables debug/validation by default
   

## Adding a New Test

1. Create `VulkanWrapper/tests/<Category>/MyTests.cpp`
2. Add to `VulkanWrapper/tests/CMakeLists.txt`:
```cmake
add_executable(MyTests <Category>/MyTests.cpp)
target_link_libraries(MyTests PRIVATE TestUtils VulkanWrapperCoreLibrary GTest::gtest GTest::gtest_main)
gtest_discover_tests(MyTests)
```

## Definition of Done
