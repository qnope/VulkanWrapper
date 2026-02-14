#include "utils/create_gpu.hpp"
#include "VulkanWrapper/Memory/StagingBufferManager.h"
#include "VulkanWrapper/Model/Material/BindlessMaterialManager.h"
#include "VulkanWrapper/Model/Material/ColoredMaterialHandler.h"
#include "VulkanWrapper/Model/Material/TexturedMaterialHandler.h"
#include "VulkanWrapper/Utils/Error.h"
#include <assimp/material.h>
#include <gtest/gtest.h>

using namespace vw::Model::Material;

class BindlessMaterialManagerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &gpu = vw::tests::create_gpu();
        m_staging = std::make_shared<vw::StagingBufferManager>(gpu.device,
                                                               gpu.allocator);
        m_manager = std::make_unique<BindlessMaterialManager>(
            gpu.device, gpu.allocator, m_staging);

        m_test_image_path = std::filesystem::path(__FILE__)
                                .parent_path()
                                .parent_path()
                                .parent_path()
                                .parent_path() /
                            "Images";
    }

    std::shared_ptr<vw::StagingBufferManager> m_staging;
    std::unique_ptr<BindlessMaterialManager> m_manager;
    std::filesystem::path m_test_image_path;
};

TEST_F(BindlessMaterialManagerTest, RegisterColoredHandler) {
    m_manager->register_handler<ColoredMaterialHandler>();

    auto *handler = m_manager->handler(colored_material_tag);
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->tag(), colored_material_tag);
}

TEST_F(BindlessMaterialManagerTest, RegisterTexturedHandler) {
    m_manager->register_handler<TexturedMaterialHandler>(
        m_manager->texture_manager());

    auto *handler = m_manager->handler(textured_material_tag);
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->tag(), textured_material_tag);
}

TEST_F(BindlessMaterialManagerTest, RegisterMultipleHandlers) {
    m_manager->register_handler<ColoredMaterialHandler>();
    m_manager->register_handler<TexturedMaterialHandler>(
        m_manager->texture_manager());

    EXPECT_NE(m_manager->handler(colored_material_tag), nullptr);
    EXPECT_NE(m_manager->handler(textured_material_tag), nullptr);
}

TEST_F(BindlessMaterialManagerTest, UnknownHandlerReturnsNull) {
    MaterialTypeTag unknown_tag{9999};
    EXPECT_EQ(m_manager->handler(unknown_tag), nullptr);
}

TEST_F(BindlessMaterialManagerTest, ThrowsWhenNoHandlersRegistered) {
    aiMaterial material;

    EXPECT_THROW(m_manager->create_material(&material, ""), vw::LogicException);
}

TEST_F(BindlessMaterialManagerTest, CreateColoredMaterial) {
    m_manager->register_handler<ColoredMaterialHandler>();

    aiMaterial material;
    aiColor4D color(1.0f, 0.0f, 0.0f, 1.0f);
    material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    auto result = m_manager->create_material(&material, "");

    EXPECT_EQ(result.material_type, colored_material_tag);
    EXPECT_NE(result.buffer_address, 0);
}

TEST_F(BindlessMaterialManagerTest, CreateTexturedMaterial) {
    m_manager->register_handler<TexturedMaterialHandler>(
        m_manager->texture_manager());
    m_manager->register_handler<ColoredMaterialHandler>();

    aiMaterial material;
    aiString texture_path("image_test.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    auto result = m_manager->create_material(&material, m_test_image_path);

    // Textured handler has higher priority, so it should be used
    EXPECT_EQ(result.material_type, textured_material_tag);
    EXPECT_NE(result.buffer_address, 0);
}

TEST_F(BindlessMaterialManagerTest, FallbackToColoredWhenTextureNotFound) {
    m_manager->register_handler<TexturedMaterialHandler>(
        m_manager->texture_manager());
    m_manager->register_handler<ColoredMaterialHandler>();

    aiMaterial material;
    // Set a texture that doesn't exist
    aiString texture_path("nonexistent.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
    // Also set a color for fallback
    aiColor4D color(0.5f, 0.5f, 0.5f, 1.0f);
    material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    auto result = m_manager->create_material(&material, m_test_image_path);

    // Should fall back to colored handler
    EXPECT_EQ(result.material_type, colored_material_tag);
}

TEST_F(BindlessMaterialManagerTest, PriorityOrderHigherFirst) {
    // Register in opposite order - colored first, then textured
    m_manager->register_handler<ColoredMaterialHandler>();
    m_manager->register_handler<TexturedMaterialHandler>(
        m_manager->texture_manager());

    aiMaterial material;
    aiString texture_path("image_test.png");
    material.AddProperty(&texture_path,
                         AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
    aiColor4D color(1.0f, 0.0f, 0.0f, 1.0f);
    material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    auto result = m_manager->create_material(&material, m_test_image_path);

    // Textured should still be tried first (higher priority)
    EXPECT_EQ(result.material_type, textured_material_tag);
}

TEST_F(BindlessMaterialManagerTest, UploadAll) {
    m_manager->register_handler<ColoredMaterialHandler>();
    m_manager->register_handler<TexturedMaterialHandler>(
        m_manager->texture_manager());

    aiMaterial material1;
    aiColor4D color(1.0f, 0.0f, 0.0f, 1.0f);
    material1.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    aiMaterial material2;
    aiString texture_path("image_test.png");
    material2.AddProperty(&texture_path,
                          AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    m_manager->create_material(&material1, "");
    m_manager->create_material(&material2, m_test_image_path);

    m_manager->upload_all();

    // After upload, handlers should have valid buffer addresses
    auto *colored = m_manager->handler(colored_material_tag);
    auto *textured = m_manager->handler(textured_material_tag);
    ASSERT_NE(colored, nullptr);
    ASSERT_NE(textured, nullptr);
    EXPECT_NE(colored->buffer_address(), vk::DeviceAddress{0});
    EXPECT_NE(textured->buffer_address(), vk::DeviceAddress{0});
}

TEST_F(BindlessMaterialManagerTest, GetResourcesAggregatesAllHandlers) {
    m_manager->register_handler<ColoredMaterialHandler>();
    m_manager->register_handler<TexturedMaterialHandler>(
        m_manager->texture_manager());

    aiMaterial material1;
    aiColor4D color(1.0f, 0.0f, 0.0f, 1.0f);
    material1.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    aiMaterial material2;
    aiString texture_path("image_test.png");
    material2.AddProperty(&texture_path,
                          AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

    m_manager->create_material(&material1, "");
    m_manager->create_material(&material2, m_test_image_path);
    m_manager->upload_all();

    auto resources = m_manager->get_resources();

    // Should have resources from both handlers
    EXPECT_FALSE(resources.empty());
}

TEST_F(BindlessMaterialManagerTest, TextureManagerAccessible) {
    auto &texture_manager = m_manager->texture_manager();

    // Should be able to access texture manager for external use
    EXPECT_EQ(texture_manager.texture_count(), 0);
}

TEST_F(BindlessMaterialManagerTest, ConstTextureManagerAccessible) {
    const auto &const_manager = *m_manager;
    const auto &texture_manager = const_manager.texture_manager();

    EXPECT_EQ(texture_manager.texture_count(), 0);
}

TEST_F(BindlessMaterialManagerTest, CreateMultipleMaterialsOfSameType) {
    m_manager->register_handler<ColoredMaterialHandler>();

    std::vector<Material> materials;
    for (int i = 0; i < 10; ++i) {
        aiMaterial material;
        aiColor4D color(static_cast<float>(i) / 10.0f, 0.0f, 0.0f, 1.0f);
        material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

        materials.push_back(m_manager->create_material(&material, ""));
        EXPECT_EQ(materials.back().material_type, colored_material_tag);
    }

    // Verify all buffer_address values are unique and evenly spaced
    auto *handler = m_manager->handler(colored_material_tag);
    auto stride = handler->stride();
    for (int i = 1; i < 10; ++i) {
        EXPECT_EQ(materials[i].buffer_address -
                      materials[i - 1].buffer_address,
                  stride)
            << "Material " << i << " address not spaced by stride";
    }
}

TEST_F(BindlessMaterialManagerTest, ConstHandlerAccess) {
    m_manager->register_handler<ColoredMaterialHandler>();

    const auto &const_manager = *m_manager;
    const auto *handler = const_manager.handler(colored_material_tag);

    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->tag(), colored_material_tag);
}

TEST_F(BindlessMaterialManagerTest, MaterialAddress) {
    m_manager->register_handler<ColoredMaterialHandler>();

    aiMaterial material;
    aiColor4D color(1.0f, 0.0f, 0.0f, 1.0f);
    material.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);

    auto result = m_manager->create_material(&material, "");

    auto *handler = m_manager->handler(colored_material_tag);
    ASSERT_NE(handler, nullptr);

    // First material's buffer_address should equal the handler's base address
    EXPECT_EQ(result.buffer_address, handler->buffer_address());
}

TEST_F(BindlessMaterialManagerTest, HandlersViewReturnsRegisteredHandlers) {
    m_manager->register_handler<ColoredMaterialHandler>();
    m_manager->register_handler<TexturedMaterialHandler>(
        m_manager->texture_manager());

    int count = 0;
    bool found_colored = false;
    bool found_textured = false;
    for (auto [tag, handler] : m_manager->handlers()) {
        ASSERT_NE(handler, nullptr);
        if (tag == colored_material_tag)
            found_colored = true;
        if (tag == textured_material_tag)
            found_textured = true;
        ++count;
    }

    EXPECT_EQ(count, 2);
    EXPECT_TRUE(found_colored);
    EXPECT_TRUE(found_textured);
}

TEST_F(BindlessMaterialManagerTest, HandlersViewEmptyWhenNoHandlers) {
    int count = 0;
    for ([[maybe_unused]] auto [tag, handler] : m_manager->handlers()) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST_F(BindlessMaterialManagerTest, MaterialAddressMultipleMaterials) {
    m_manager->register_handler<ColoredMaterialHandler>();

    constexpr int count = 5;
    std::vector<Material> materials;
    materials.reserve(count);

    for (int i = 0; i < count; ++i) {
        aiMaterial mat;
        aiColor4D color(static_cast<float>(i) / count, 0.0f, 0.0f, 1.0f);
        mat.AddProperty(&color, 1, AI_MATKEY_COLOR_DIFFUSE);
        materials.push_back(m_manager->create_material(&mat, ""));
    }

    auto *handler = m_manager->handler(colored_material_tag);
    ASSERT_NE(handler, nullptr);

    auto base_addr = handler->buffer_address();
    auto stride = handler->stride();

    for (int i = 0; i < count; ++i) {
        EXPECT_EQ(materials[i].buffer_address, base_addr + i * stride)
            << "Material " << i << " address mismatch";
    }

    // Verify addresses are evenly spaced
    for (int i = 1; i < count; ++i) {
        EXPECT_EQ(materials[i].buffer_address -
                      materials[i - 1].buffer_address,
                  stride);
    }
}
