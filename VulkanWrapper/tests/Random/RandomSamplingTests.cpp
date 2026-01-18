#include "utils/create_gpu.hpp"

#include "VulkanWrapper/Random/NoiseTexture.h"
#include "VulkanWrapper/Random/RandomSamplingBuffer.h"

#include <gtest/gtest.h>

// ============================================================================
// DualRandomSample generation tests
// ============================================================================

TEST(RandomSamplingTest, GenerateDualRandomSampleValuesInRange) {
    auto samples = vw::generate_hemisphere_samples();

    for (std::size_t i = 0; i < vw::DUAL_SAMPLE_COUNT; ++i) {
        EXPECT_GE(samples.samples[i].x, 0.0f)
            << "Sample " << i << " x component is negative";
        EXPECT_LT(samples.samples[i].x, 1.0f)
            << "Sample " << i << " x component is >= 1";
        EXPECT_GE(samples.samples[i].y, 0.0f)
            << "Sample " << i << " y component is negative";
        EXPECT_LT(samples.samples[i].y, 1.0f)
            << "Sample " << i << " y component is >= 1";
    }
}

TEST(RandomSamplingTest, GenerateDualRandomSampleReproducibleWithSeed) {
    auto samples1 = vw::generate_hemisphere_samples(42);
    auto samples2 = vw::generate_hemisphere_samples(42);

    // Same seed should produce identical samples
    for (std::size_t i = 0; i < vw::DUAL_SAMPLE_COUNT; ++i) {
        EXPECT_EQ(samples1.samples[i].x, samples2.samples[i].x)
            << "Sample " << i << " x component differs with same seed";
        EXPECT_EQ(samples1.samples[i].y, samples2.samples[i].y)
            << "Sample " << i << " y component differs with same seed";
    }
}

TEST(RandomSamplingTest, GenerateDualRandomSampleDifferentWithDifferentSeed) {
    auto samples1 = vw::generate_hemisphere_samples(42);
    auto samples2 = vw::generate_hemisphere_samples(123);

    // Different seeds should produce different samples
    bool any_different = false;
    for (std::size_t i = 0; i < vw::DUAL_SAMPLE_COUNT; ++i) {
        if (samples1.samples[i] != samples2.samples[i]) {
            any_different = true;
            break;
        }
    }
    EXPECT_TRUE(any_different)
        << "Different seeds should produce different samples";
}

// ============================================================================
// DualRandomSampleBuffer tests
// ============================================================================

TEST(RandomSamplingTest, CreateDualRandomSampleBuffer) {
    auto &gpu = vw::tests::create_gpu();

    auto buffer = vw::create_hemisphere_samples_buffer(*gpu.allocator);

    EXPECT_NE(buffer.handle(), vk::Buffer{});
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_EQ(buffer.size_bytes(), sizeof(vw::DualRandomSample));
}

TEST(RandomSamplingTest, DualRandomSampleBufferContainsValidData) {
    auto &gpu = vw::tests::create_gpu();

    auto buffer = vw::create_hemisphere_samples_buffer(*gpu.allocator, 42);

    // Read back and verify values are in [0, 1)
    auto data = buffer.read_as_vector(0, 1);
    ASSERT_EQ(data.size(), 1);

    for (std::size_t i = 0; i < vw::DUAL_SAMPLE_COUNT; ++i) {
        EXPECT_GE(data[0].samples[i].x, 0.0f);
        EXPECT_LT(data[0].samples[i].x, 1.0f);
        EXPECT_GE(data[0].samples[i].y, 0.0f);
        EXPECT_LT(data[0].samples[i].y, 1.0f);
    }
}

TEST(RandomSamplingTest, DualRandomSampleBufferReproducibleWithSeed) {
    auto &gpu = vw::tests::create_gpu();

    auto buffer1 = vw::create_hemisphere_samples_buffer(*gpu.allocator, 42);
    auto buffer2 = vw::create_hemisphere_samples_buffer(*gpu.allocator, 42);

    auto data1 = buffer1.read_as_vector(0, 1);
    auto data2 = buffer2.read_as_vector(0, 1);

    for (std::size_t i = 0; i < vw::DUAL_SAMPLE_COUNT; ++i) {
        EXPECT_EQ(data1[0].samples[i], data2[0].samples[i])
            << "Buffer data differs at sample " << i << " with same seed";
    }
}

// ============================================================================
// NoiseTexture tests
// ============================================================================

TEST(RandomSamplingTest, CreateNoiseTexture) {
    auto &gpu = vw::tests::create_gpu();

    vw::NoiseTexture noise(gpu.device, gpu.allocator, gpu.queue());

    EXPECT_NE(noise.image().handle(), vk::Image{});
    EXPECT_NE(noise.view().handle(), vk::ImageView{});
    EXPECT_NE(noise.sampler().handle(), vk::Sampler{});
}

TEST(RandomSamplingTest, NoiseTextureHasCorrectDimensions) {
    auto &gpu = vw::tests::create_gpu();

    vw::NoiseTexture noise(gpu.device, gpu.allocator, gpu.queue());

    auto extent = noise.image().extent2D();
    EXPECT_EQ(extent.width, vw::NOISE_TEXTURE_SIZE);
    EXPECT_EQ(extent.height, vw::NOISE_TEXTURE_SIZE);
}

TEST(RandomSamplingTest, NoiseTextureHasCorrectFormat) {
    auto &gpu = vw::tests::create_gpu();

    vw::NoiseTexture noise(gpu.device, gpu.allocator, gpu.queue());

    EXPECT_EQ(noise.image().format(), vk::Format::eR32G32Sfloat);
}

TEST(RandomSamplingTest, NoiseTextureCombinedImageIsValid) {
    auto &gpu = vw::tests::create_gpu();

    vw::NoiseTexture noise(gpu.device, gpu.allocator, gpu.queue());

    auto combined = noise.combined_image();
    EXPECT_NE(combined.image(), vk::Image{});
    EXPECT_NE(combined.image_view(), vk::ImageView{});
    EXPECT_NE(combined.sampler(), vk::Sampler{});
}

TEST(RandomSamplingTest, NoiseTextureWithSeed) {
    auto &gpu = vw::tests::create_gpu();

    // Should not throw - just verify construction works with seed
    vw::NoiseTexture noise(gpu.device, gpu.allocator, gpu.queue(), 42);

    EXPECT_NE(noise.image().handle(), vk::Image{});
}

// ============================================================================
// Constants tests
// ============================================================================

TEST(RandomSamplingTest, ConstantsHaveExpectedValues) {
    EXPECT_EQ(vw::DUAL_SAMPLE_COUNT, 4096);
    EXPECT_EQ(vw::NOISE_TEXTURE_SIZE, 4096);
}
