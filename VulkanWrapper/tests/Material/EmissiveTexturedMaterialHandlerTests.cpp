#include <assimp/material.h>
#include <gtest/gtest.h>
import vw.test;

using namespace vw::Model::Material;

class EmissiveTexturedMaterialHandlerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        m_staging = std::make_shared<vw::StagingBufferManager>(gpu.device,
                                                               gpu.allocator);
        m_texture_manager = std::make_unique<BindlessTextureManager>(
            gpu.device, gpu.allocator, m_staging);
        m_handler = EmissiveTexturedMaterialHandler::Base::create<
            EmissiveTexturedMaterialHandler>(gpu.device, gpu.allocator,
                                             *m_texture_manager);
        m_test_image_path = std::filesystem::path(__FILE__)
                                .parent_path()
                                .parent_path()
                                .parent_path()
                                .parent_path() /
                            "Images";
    }

    std::shared_ptr<vw::StagingBufferManager> m_staging;
    std::unique_ptr<BindlessTextureManager> m_texture_manager;
    std::unique_ptr<EmissiveTexturedMaterialHandler> m_handler;
    std::filesystem::path m_test_image_path;
};

TEST_F(EmissiveTexturedMaterialHandlerTest, BrdfPathIsCorrect) {
    EXPECT_EQ(m_handler->brdf_path(), "Material/brdf_emissive_textured.glsl");
}

TEST_F(EmissiveTexturedMaterialHandlerTest, TagReturnsEmissiveTexturedTag) {
    EXPECT_EQ(m_handler->tag(), emissive_textured_material_tag);
}

TEST_F(EmissiveTexturedMaterialHandlerTest,
       PriorityReturnsEmissiveTexturedPriority) {
    EXPECT_EQ(m_handler->priority(), emissive_textured_material_priority);
}

TEST_F(EmissiveTexturedMaterialHandlerTest, StrideMatchesDataSize) {
    EXPECT_EQ(m_handler->stride(), sizeof(EmissiveTexturedMaterialData));
}

TEST_F(EmissiveTexturedMaterialHandlerTest,
       CreateMaterialReturnsValidMaterial) {
    auto texture = m_test_image_path / "image_test.png";
    auto result = m_handler->create_material(texture, 100.0f);

    EXPECT_EQ(result.material_type, emissive_textured_material_tag);
}

TEST_F(EmissiveTexturedMaterialHandlerTest,
       CreateMaterialReturnsValidBufferAddress) {
    auto texture = m_test_image_path / "image_test.png";
    auto result = m_handler->create_material(texture, 100.0f);

    EXPECT_NE(result.buffer_address, 0);
}

TEST_F(EmissiveTexturedMaterialHandlerTest,
       MultipleMaterialsHaveCorrectSpacing) {
    auto texture = m_test_image_path / "image_test.png";
    auto result1 = m_handler->create_material(texture, 50.0f);
    auto result2 = m_handler->create_material(texture, 100.0f);

    EXPECT_EQ(result2.buffer_address - result1.buffer_address,
              sizeof(EmissiveTexturedMaterialData));
}

TEST_F(EmissiveTexturedMaterialHandlerTest, AdditionalDescriptorSetIsPresent) {
    auto texture = m_test_image_path / "image_test.png";
    std::ignore = m_handler->create_material(texture, 100.0f);
    m_handler->upload();

    auto ds = m_handler->additional_descriptor_set();
    ASSERT_TRUE(ds.has_value());
    EXPECT_NE(*ds, vk::DescriptorSet{});
}

TEST_F(EmissiveTexturedMaterialHandlerTest,
       AdditionalDescriptorSetLayoutIsPresent) {
    auto layout = m_handler->additional_descriptor_set_layout();
    EXPECT_NE(layout, nullptr);
}

TEST_F(EmissiveTexturedMaterialHandlerTest, TryCreateWithEmissiveTexture) {
    aiMaterial material;
    aiString texture_path("image_test.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, 0));

    auto result = m_handler->try_create(&material, m_test_image_path);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->material_type, emissive_textured_material_tag);
    EXPECT_NE(result->buffer_address, 0);
}

TEST_F(EmissiveTexturedMaterialHandlerTest,
       TryCreateRejectsWhenNoEmissiveTexture) {
    aiMaterial material;
    // No emissive texture property set

    auto result = m_handler->try_create(&material, "");

    EXPECT_FALSE(result.has_value());
}

TEST_F(EmissiveTexturedMaterialHandlerTest,
       TryCreateRejectsWhenTextureFileNotFound) {
    aiMaterial material;
    aiString texture_path("nonexistent_texture.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, 0));

    auto result = m_handler->try_create(&material, "/tmp");

    EXPECT_FALSE(result.has_value());
}

TEST_F(EmissiveTexturedMaterialHandlerTest, TryCreateWithCustomIntensity) {
    aiMaterial material;
    aiString texture_path("image_test.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, 0));
    float intensity = 50.0f;
    material.AddProperty(&intensity, 1, AI_MATKEY_EMISSIVE_INTENSITY);

    auto result = m_handler->try_create(&material, m_test_image_path);

    ASSERT_TRUE(result.has_value());
}

TEST_F(EmissiveTexturedMaterialHandlerTest, UploadAfterMaterialCreation) {
    auto texture = m_test_image_path / "image_test.png";
    std::ignore = m_handler->create_material(texture, 100.0f);
    m_handler->upload();

    auto resources = m_handler->get_resources();
    EXPECT_FALSE(resources.empty());
}

TEST_F(EmissiveTexturedMaterialHandlerTest, BufferAddressAfterUpload) {
    auto texture = m_test_image_path / "image_test.png";
    std::ignore = m_handler->create_material(texture, 100.0f);
    m_handler->upload();

    EXPECT_NE(m_handler->buffer_address(), vk::DeviceAddress{0});
}

TEST_F(EmissiveTexturedMaterialHandlerTest,
       MaterialAddressMatchesBufferAddress) {
    auto texture = m_test_image_path / "image_test.png";
    auto result = m_handler->create_material(texture, 100.0f);

    // First material at offset 0 should match handler's buffer_address
    EXPECT_EQ(result.buffer_address, m_handler->buffer_address());
}

TEST_F(EmissiveTexturedMaterialHandlerTest,
       EmissivePriorityHigherThanTextured) {
    EXPECT_GT(static_cast<int>(emissive_textured_material_priority),
              static_cast<int>(textured_material_priority));
}
