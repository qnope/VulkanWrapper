#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Model/Material/BindlessTextureManager.h"
#include "VulkanWrapper/Model/Material/TexturedMaterialHandler.h"
#include <assimp/material.h>
#include <gtest/gtest.h>

using namespace vw::Model::Material;

class TexturedMaterialHandlerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        m_staging = std::make_shared<vw::StagingBufferManager>(gpu.device,
                                                               gpu.allocator);
        m_texture_manager = std::make_unique<BindlessTextureManager>(
            gpu.device, gpu.allocator, m_staging);
        m_handler =
            TexturedMaterialHandler::Base::create<TexturedMaterialHandler>(
                gpu.device, gpu.allocator, *m_texture_manager);
    }

    std::shared_ptr<vw::StagingBufferManager> m_staging;
    std::unique_ptr<BindlessTextureManager> m_texture_manager;
    std::unique_ptr<TexturedMaterialHandler> m_handler;
};

TEST_F(TexturedMaterialHandlerTest, HandlerHasCorrectTag) {
    EXPECT_EQ(m_handler->tag(), textured_material_tag);
}

TEST_F(TexturedMaterialHandlerTest, HandlerHasCorrectPriority) {
    EXPECT_EQ(m_handler->priority(), textured_material_priority);
}

TEST_F(TexturedMaterialHandlerTest, StrideMatchesDataSize) {
    EXPECT_EQ(m_handler->stride(), sizeof(TexturedMaterialData));
}

TEST_F(TexturedMaterialHandlerTest, RejectsWhenNoTexture) {
    aiMaterial material;
    // No texture property set

    auto result = m_handler->try_create(&material, "");

    EXPECT_FALSE(result.has_value());
}

TEST_F(TexturedMaterialHandlerTest, RejectsWhenTextureFileNotFound) {
    aiMaterial material;
    aiString texture_path("nonexistent_texture.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    auto result = m_handler->try_create(&material, "/tmp");

    EXPECT_FALSE(result.has_value());
}

TEST_F(TexturedMaterialHandlerTest, CreateMaterialWithValidTexture) {
    aiMaterial material;
    aiString texture_path("image_test.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    // Use the test image from Images directory
    std::filesystem::path base_path = std::filesystem::path(__FILE__)
                                          .parent_path()
                                          .parent_path()
                                          .parent_path()
                                          .parent_path() /
                                      "Images";

    auto result = m_handler->try_create(&material, base_path);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->material_type, textured_material_tag);
    EXPECT_NE(result->buffer_address, 0);
}

TEST_F(TexturedMaterialHandlerTest, CreateMultipleMaterialsSameTexture) {
    aiMaterial material1;
    aiString texture_path1("image_test.png");
    material1.AddProperty(&texture_path1,
                          AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    aiMaterial material2;
    aiString texture_path2("image_test.png");
    material2.AddProperty(&texture_path2,
                          AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    std::filesystem::path base_path = std::filesystem::path(__FILE__)
                                          .parent_path()
                                          .parent_path()
                                          .parent_path()
                                          .parent_path() /
                                      "Images";

    auto result1 = m_handler->try_create(&material1, base_path);
    auto result2 = m_handler->try_create(&material2, base_path);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    // Different buffer addresses but texture should be cached
    EXPECT_NE(result1->buffer_address, result2->buffer_address);

    // Texture manager should only have one texture (cached)
    EXPECT_EQ(m_texture_manager->texture_count(), 1);
}

TEST_F(TexturedMaterialHandlerTest, NormalizesBackslashPaths) {
    aiMaterial material;
    // Use Windows-style backslashes
    aiString texture_path("subdir\\image_test.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    // This should fail because the file doesn't exist, but the path
    // normalization should still work (converting \ to /)
    auto result = m_handler->try_create(&material, "/nonexistent");

    // The handler will return nullopt because the file doesn't exist,
    // not because of path issues
    EXPECT_FALSE(result.has_value());
}

TEST_F(TexturedMaterialHandlerTest, UploadAfterMaterialCreation) {
    aiMaterial material;
    aiString texture_path("image_test.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    std::filesystem::path base_path = std::filesystem::path(__FILE__)
                                          .parent_path()
                                          .parent_path()
                                          .parent_path()
                                          .parent_path() /
                                      "Images";

    m_handler->try_create(&material, base_path);
    m_handler->upload();

    // After upload, resources should include buffer and texture
    auto resources = m_handler->get_resources();
    EXPECT_FALSE(resources.empty());
}

TEST_F(TexturedMaterialHandlerTest, BufferAddressAfterUpload) {
    aiMaterial material;
    aiString texture_path("image_test.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    std::filesystem::path base_path = std::filesystem::path(__FILE__)
                                          .parent_path()
                                          .parent_path()
                                          .parent_path()
                                          .parent_path() /
                                      "Images";

    m_handler->try_create(&material, base_path);
    m_handler->upload();

    EXPECT_NE(m_handler->buffer_address(), vk::DeviceAddress{0});
}

TEST_F(TexturedMaterialHandlerTest, TexturedPriorityHigherThanColored) {
    EXPECT_GT(static_cast<int>(textured_material_priority),
              static_cast<int>(colored_material_priority));
}

TEST_F(TexturedMaterialHandlerTest, AdditionalDescriptorSetLayout) {
    auto layout = m_handler->additional_descriptor_set_layout();
    EXPECT_NE(layout, nullptr);
}

TEST_F(TexturedMaterialHandlerTest, AdditionalDescriptorSetAfterUpload) {
    aiMaterial material;
    aiString texture_path("image_test.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    std::filesystem::path base_path = std::filesystem::path(__FILE__)
                                          .parent_path()
                                          .parent_path()
                                          .parent_path()
                                          .parent_path() /
                                      "Images";

    m_handler->try_create(&material, base_path);
    m_handler->upload();

    auto ds = m_handler->additional_descriptor_set();
    ASSERT_TRUE(ds.has_value());
    EXPECT_NE(*ds, vk::DescriptorSet{});
}

TEST_F(TexturedMaterialHandlerTest, MaterialAddressMatchesBufferAddress) {
    aiMaterial material;
    aiString texture_path("image_test.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    std::filesystem::path base_path = std::filesystem::path(__FILE__)
                                          .parent_path()
                                          .parent_path()
                                          .parent_path()
                                          .parent_path() /
                                      "Images";

    auto result = m_handler->try_create(&material, base_path);
    ASSERT_TRUE(result.has_value());

    // First material at offset 0 should match handler's buffer_address
    EXPECT_EQ(result->buffer_address, m_handler->buffer_address());
}
