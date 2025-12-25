#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Model/Material/BindlessTextureManager.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialHandler.h"
#include <assimp/material.h>
#include <gtest/gtest.h>

using namespace vw::Model::Material;

class ColoredMaterialHandlerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        m_staging = std::make_shared<vw::StagingBufferManager>(gpu.device,
                                                               gpu.allocator);
        m_texture_manager = std::make_unique<BindlessTextureManager>(
            gpu.device, gpu.allocator, m_staging);
        m_handler = ColoredMaterialHandler::create<ColoredMaterialHandler>(
            gpu.device, gpu.allocator, *m_texture_manager);
    }

    std::shared_ptr<vw::StagingBufferManager> m_staging;
    std::unique_ptr<BindlessTextureManager> m_texture_manager;
    std::unique_ptr<ColoredMaterialHandler> m_handler;
};

TEST_F(ColoredMaterialHandlerTest, HandlerHasCorrectTag) {
    EXPECT_EQ(m_handler->tag(), colored_material_tag);
}

TEST_F(ColoredMaterialHandlerTest, HandlerHasCorrectPriority) {
    EXPECT_EQ(m_handler->priority(), colored_material_priority);
}

TEST_F(ColoredMaterialHandlerTest, HandlerHasValidLayout) {
    auto layout = m_handler->layout();
    EXPECT_NE(layout, nullptr);
}

TEST_F(ColoredMaterialHandlerTest, HandlerHasValidDescriptorSet) {
    auto descriptor_set = m_handler->descriptor_set();
    EXPECT_TRUE(descriptor_set);
}

TEST_F(ColoredMaterialHandlerTest, CreateMaterialWithDiffuseColor) {
    aiMaterial material;
    aiColor4D color(1.0f, 0.5f, 0.25f, 1.0f);
    material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    auto result = m_handler->try_create(&material, "");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->material_type, colored_material_tag);
    EXPECT_EQ(result->material_index, 0);
}

TEST_F(ColoredMaterialHandlerTest, CreateMaterialWithDefaultColor) {
    aiMaterial material;
    // No color property set - should use default (0.5, 0.5, 0.5, 1.0)

    auto result = m_handler->try_create(&material, "");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->material_type, colored_material_tag);
    EXPECT_EQ(result->material_index, 0);
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

    EXPECT_EQ(result1->material_index, 0);
    EXPECT_EQ(result2->material_index, 1);
    EXPECT_EQ(result3->material_index, 2);
}

TEST_F(ColoredMaterialHandlerTest, UploadCreatesSSBO) {
    aiMaterial material;
    aiColor4D color(1.0f, 0.0f, 0.0f, 1.0f);
    material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    m_handler->try_create(&material, "");
    m_handler->upload();

    // After upload, resources should include the SSBO
    auto resources = m_handler->get_resources();
    EXPECT_FALSE(resources.empty());
}

TEST_F(ColoredMaterialHandlerTest, NoResourcesBeforeUpload) {
    aiMaterial material;
    m_handler->try_create(&material, "");

    // Before upload, SSBO not allocated
    auto resources = m_handler->get_resources();
    EXPECT_TRUE(resources.empty());
}

TEST_F(ColoredMaterialHandlerTest, UploadWithNoMaterialsDoesNothing) {
    // No materials created - upload should be a no-op
    m_handler->upload();

    auto resources = m_handler->get_resources();
    EXPECT_TRUE(resources.empty());
}
