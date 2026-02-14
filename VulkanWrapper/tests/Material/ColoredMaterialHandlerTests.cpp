#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Model/Material/ColoredMaterialHandler.h"
#include <assimp/material.h>
#include <gtest/gtest.h>

using namespace vw::Model::Material;

class ColoredMaterialHandlerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        m_handler = ColoredMaterialHandler::create<ColoredMaterialHandler>(
            gpu.device, gpu.allocator);
    }

    std::unique_ptr<ColoredMaterialHandler> m_handler;
};

TEST_F(ColoredMaterialHandlerTest, HandlerHasCorrectTag) {
    EXPECT_EQ(m_handler->tag(), colored_material_tag);
}

TEST_F(ColoredMaterialHandlerTest, HandlerHasCorrectPriority) {
    EXPECT_EQ(m_handler->priority(), colored_material_priority);
}

TEST_F(ColoredMaterialHandlerTest, StrideMatchesDataSize) {
    EXPECT_EQ(m_handler->stride(), sizeof(ColoredMaterialData));
}

TEST_F(ColoredMaterialHandlerTest, CreateMaterialWithDiffuseColor) {
    aiMaterial material;
    aiColor4D color(1.0f, 0.5f, 0.25f, 1.0f);
    material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    auto result = m_handler->try_create(&material, "");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->material_type, colored_material_tag);
    EXPECT_NE(result->buffer_address, 0);
}

TEST_F(ColoredMaterialHandlerTest, CreateMaterialWithDefaultColor) {
    aiMaterial material;
    // No color property set - should use default (0.5, 0.5, 0.5, 1.0)

    auto result = m_handler->try_create(&material, "");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->material_type, colored_material_tag);
    EXPECT_NE(result->buffer_address, 0);
}

TEST_F(ColoredMaterialHandlerTest, CreateMultipleMaterials) {
    aiMaterial material1;
    aiColor4D color1(1.0f, 0.0f, 0.0f, 1.0f);
    material1.AddProperty(&color1, 1, AI_MATKEY_COLOR_DIFFUSE);

    aiMaterial material2;
    aiColor4D color2(0.0f, 1.0f, 0.0f, 1.0f);
    material2.AddProperty(&color2, 1, AI_MATKEY_COLOR_DIFFUSE);

    aiMaterial material3;
    aiColor4D color3(0.0f, 0.0f, 1.0f, 1.0f);
    material3.AddProperty(&color3, 1, AI_MATKEY_COLOR_DIFFUSE);

    auto result1 = m_handler->try_create(&material1, "");
    auto result2 = m_handler->try_create(&material2, "");
    auto result3 = m_handler->try_create(&material3, "");

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_TRUE(result3.has_value());

    // Each material should have a different buffer_address (base + i * stride)
    EXPECT_NE(result1->buffer_address, result2->buffer_address);
    EXPECT_NE(result2->buffer_address, result3->buffer_address);
    EXPECT_NE(result1->buffer_address, result3->buffer_address);

    auto stride = m_handler->stride();
    EXPECT_EQ(result2->buffer_address - result1->buffer_address, stride);
    EXPECT_EQ(result3->buffer_address - result2->buffer_address, stride);
}

TEST_F(ColoredMaterialHandlerTest, UploadCreatesBuffer) {
    aiMaterial material;
    aiColor4D color(1.0f, 0.0f, 0.0f, 1.0f);
    material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    m_handler->try_create(&material, "");
    m_handler->upload();

    // After upload, resources should include the buffer
    auto resources = m_handler->get_resources();
    EXPECT_FALSE(resources.empty());
}

TEST_F(ColoredMaterialHandlerTest, BufferAddressAvailableWithoutUpload) {
    aiMaterial material;
    aiColor4D color(1.0f, 0.0f, 0.0f, 1.0f);
    material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    m_handler->try_create(&material, "");

    // SSBO is pre-allocated at construction, buffer_address works without upload
    EXPECT_NE(m_handler->buffer_address(), vk::DeviceAddress{0});
}

TEST_F(ColoredMaterialHandlerTest, ResourcesAvailableAfterConstruction) {
    aiMaterial material;
    m_handler->try_create(&material, "");

    // SSBO is pre-allocated at construction, resources should not be empty
    auto resources = m_handler->get_resources();
    EXPECT_FALSE(resources.empty());
}

TEST_F(ColoredMaterialHandlerTest, UploadWithNoMaterialsDoesNothing) {
    // No materials created - upload should be a no-op
    m_handler->upload();

    // SSBO is pre-allocated, so resources always include the buffer
    auto resources = m_handler->get_resources();
    EXPECT_FALSE(resources.empty());
}

TEST_F(ColoredMaterialHandlerTest, AdditionalDescriptorSetIsNullopt) {
    EXPECT_FALSE(m_handler->additional_descriptor_set().has_value());
}

TEST_F(ColoredMaterialHandlerTest, AdditionalDescriptorSetLayoutIsNull) {
    EXPECT_EQ(m_handler->additional_descriptor_set_layout(), nullptr);
}

TEST_F(ColoredMaterialHandlerTest, MaterialAddressMatchesBufferAddress) {
    aiMaterial material;
    aiColor4D color(1.0f, 0.0f, 0.0f, 1.0f);
    material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    auto result = m_handler->try_create(&material, "");
    ASSERT_TRUE(result.has_value());

    // First material at offset 0 should match handler's buffer_address
    EXPECT_EQ(result->buffer_address, m_handler->buffer_address());
}

TEST_F(ColoredMaterialHandlerTest, MaterialAddressesEvenlySpaced) {
    std::vector<Material> materials;
    for (int i = 0; i < 3; ++i) {
        aiMaterial material;
        aiColor4D color(static_cast<float>(i) / 3.0f, 0.0f, 0.0f, 1.0f);
        material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);
        auto result = m_handler->try_create(&material, "");
        ASSERT_TRUE(result.has_value());
        materials.push_back(*result);
    }

    auto base = m_handler->buffer_address();
    auto stride = m_handler->stride();
    for (uint32_t i = 0; i < 3; ++i) {
        EXPECT_EQ(materials[i].buffer_address, base + i * stride);
    }
}
