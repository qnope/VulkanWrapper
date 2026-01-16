// Template: Basic Vulkan Test
// Usage: Copy this file and modify for your specific test

#include "utils/create_gpu.hpp"
#include <gtest/gtest.h>

// Include the headers for what you're testing
// #include "VulkanWrapper/YourModule/YourClass.h"

namespace {

// ============================================================================
// Test Constants
// ============================================================================

constexpr uint32_t TEST_WIDTH = 64;
constexpr uint32_t TEST_HEIGHT = 64;

// ============================================================================
// Simple Tests (no fixture needed)
// ============================================================================

TEST(YourClassTest, CreatesWithValidParameters) {
    auto &gpu = vw::tests::create_gpu();

    // Arrange
    // ... setup test data ...

    // Act
    // auto result = vw::YourClass(...);

    // Assert
    // EXPECT_TRUE(result.handle());
    // EXPECT_EQ(result.property(), expected_value);
}

TEST(YourClassTest, ThrowsOnInvalidParameters) {
    auto &gpu = vw::tests::create_gpu();

    // Arrange
    // ... setup invalid data ...

    // Act & Assert
    // EXPECT_THROW(vw::YourClass(...), vw::LogicException);
}

// ============================================================================
// Test Fixture (for tests needing common setup)
// ============================================================================

class YourClassTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        m_device = gpu.device;
        m_allocator = gpu.allocator;
        m_queue = &gpu.queue();

        // Additional setup specific to your tests
        // m_cmdPool = std::make_unique<vw::CommandPool>(
        //     vw::CommandPoolBuilder(m_device).build());
    }

    void TearDown() override {
        // Optional: wait for GPU before cleanup
        // m_device->wait_idle();
    }

    // Helper methods
    // vk::CommandBuffer allocate_command_buffer() {
    //     return m_cmdPool->allocate(1)[0];
    // }

    std::shared_ptr<vw::Device> m_device;
    std::shared_ptr<vw::Allocator> m_allocator;
    vw::Queue *m_queue;
    // std::unique_ptr<vw::CommandPool> m_cmdPool;
};

TEST_F(YourClassTest, PerformsSomeOperation) {
    // Arrange - uses fixture members
    // auto resource = m_allocator->create_resource(...);

    // Act
    // auto result = resource.operation();

    // Assert
    // EXPECT_EQ(result, expected);
}

TEST_F(YourClassTest, HandlesEdgeCase) {
    // Test boundary conditions, empty inputs, etc.
}

// ============================================================================
// Parameterized Tests (for testing multiple inputs)
// ============================================================================

struct TestParams {
    uint32_t input;
    uint32_t expected;
    const char *description;
};

class YourClassParamTest : public ::testing::TestWithParam<TestParams> {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        m_device = gpu.device;
        m_allocator = gpu.allocator;
    }

    std::shared_ptr<vw::Device> m_device;
    std::shared_ptr<vw::Allocator> m_allocator;
};

TEST_P(YourClassParamTest, ProducesExpectedOutput) {
    const auto &params = GetParam();

    // Use params.input
    // Verify params.expected
}

INSTANTIATE_TEST_SUITE_P(
    YourClassTests, YourClassParamTest,
    ::testing::Values(TestParams{1, 10, "small_input"},
                      TestParams{100, 1000, "medium_input"},
                      TestParams{10000, 100000, "large_input"}),
    [](const ::testing::TestParamInfo<TestParams> &info) {
        return info.param.description;
    });

} // namespace
