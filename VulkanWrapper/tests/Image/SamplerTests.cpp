#include <gtest/gtest.h>
#include "VulkanWrapper/Image/Sampler.h"
#include "utils/create_gpu.hpp"

TEST(SamplerTest, CreateSampler) {
    auto gpu = vw::tests::create_gpu();

    auto sampler = vw::SamplerBuilder(gpu.device).build();

    ASSERT_NE(sampler, nullptr);
    EXPECT_TRUE(sampler->handle());
}

TEST(SamplerTest, CreateMultipleSamplers) {
    auto gpu = vw::tests::create_gpu();

    auto sampler1 = vw::SamplerBuilder(gpu.device).build();
    auto sampler2 = vw::SamplerBuilder(gpu.device).build();
    auto sampler3 = vw::SamplerBuilder(gpu.device).build();

    ASSERT_NE(sampler1, nullptr);
    ASSERT_NE(sampler2, nullptr);
    ASSERT_NE(sampler3, nullptr);

    EXPECT_NE(sampler1->handle(), sampler2->handle());
    EXPECT_NE(sampler2->handle(), sampler3->handle());
}

TEST(SamplerTest, SamplerHandle) {
    auto gpu = vw::tests::create_gpu();

    auto sampler = vw::SamplerBuilder(gpu.device).build();

    ASSERT_NE(sampler, nullptr);
    EXPECT_TRUE(static_cast<bool>(sampler->handle()));
}
