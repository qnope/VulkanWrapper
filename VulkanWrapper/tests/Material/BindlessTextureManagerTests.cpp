#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Model/Material/BindlessTextureManager.h"
#include <gtest/gtest.h>

using namespace vw::Model::Material;

class BindlessTextureManagerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        m_staging = std::make_shared<vw::StagingBufferManager>(gpu.device,
                                                               gpu.allocator);
        m_manager = std::make_unique<BindlessTextureManager>(
            gpu.device, gpu.allocator, m_staging);

        m_test_image_path = std::filesystem::path(__FILE__)
                                .parent_path()
                                .parent_path()
                                .parent_path()
                                .parent_path() /
                            "Images" / "image_test.png";
    }

    std::shared_ptr<vw::StagingBufferManager> m_staging;
    std::unique_ptr<BindlessTextureManager> m_manager;
    std::filesystem::path m_test_image_path;
};

TEST_F(BindlessTextureManagerTest, MaxTexturesConstant) {
    EXPECT_EQ(BindlessTextureManager::MAX_TEXTURES, 4096);
}

TEST_F(BindlessTextureManagerTest, InitialStateHasZeroTextures) {
    EXPECT_EQ(m_manager->texture_count(), 0);
}

TEST_F(BindlessTextureManagerTest, HasValidLayout) {
    EXPECT_NE(m_manager->layout(), nullptr);
}

TEST_F(BindlessTextureManagerTest, HasValidDescriptorSet) {
    EXPECT_TRUE(m_manager->descriptor_set());
}

TEST_F(BindlessTextureManagerTest, HasValidSampler) {
    EXPECT_TRUE(m_manager->sampler());
}

TEST_F(BindlessTextureManagerTest, RegisterTexture) {
    uint32_t index = m_manager->register_texture(m_test_image_path);

    EXPECT_EQ(index, 0);
    EXPECT_EQ(m_manager->texture_count(), 1);
}

TEST_F(BindlessTextureManagerTest, RegisterSameTextureTwiceReturnsSameIndex) {
    uint32_t index1 = m_manager->register_texture(m_test_image_path);
    uint32_t index2 = m_manager->register_texture(m_test_image_path);

    EXPECT_EQ(index1, index2);
    EXPECT_EQ(m_manager->texture_count(), 1);
}

TEST_F(BindlessTextureManagerTest, GetResourcesReturnsTextureStates) {
    m_manager->register_texture(m_test_image_path);

    auto resources = m_manager->get_resources();

    ASSERT_EQ(resources.size(), 1);

    // Verify it's an ImageState
    EXPECT_TRUE(std::holds_alternative<vw::Barrier::ImageState>(resources[0]));

    auto &image_state = std::get<vw::Barrier::ImageState>(resources[0]);
    EXPECT_EQ(image_state.layout, vk::ImageLayout::eReadOnlyOptimal);
    EXPECT_EQ(image_state.stage, vk::PipelineStageFlagBits2::eFragmentShader);
    EXPECT_EQ(image_state.access, vk::AccessFlagBits2::eShaderSampledRead);
}

TEST_F(BindlessTextureManagerTest, GetResourcesEmptyWhenNoTextures) {
    auto resources = m_manager->get_resources();
    EXPECT_TRUE(resources.empty());
}

TEST_F(BindlessTextureManagerTest, UpdateDescriptors) {
    m_manager->register_texture(m_test_image_path);
    EXPECT_NO_THROW(m_manager->update_descriptors());
}
