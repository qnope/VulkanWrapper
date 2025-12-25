#include "VulkanWrapper/Model/Material/MaterialPriority.h"
#include "VulkanWrapper/Model/Material/MaterialTypeTag.h"
#include <gtest/gtest.h>
#include <unordered_set>

using namespace vw::Model::Material;

// Test tags for this file
VW_DEFINE_MATERIAL_TYPE(test_tag_1);
VW_DEFINE_MATERIAL_TYPE(test_tag_2);
VW_DEFINE_MATERIAL_TYPE(test_tag_3);

TEST(MaterialTypeTagTest, TagsAreUnique) {
    EXPECT_NE(test_tag_1.id(), test_tag_2.id());
    EXPECT_NE(test_tag_2.id(), test_tag_3.id());
    EXPECT_NE(test_tag_1.id(), test_tag_3.id());
}

TEST(MaterialTypeTagTest, TagsAreComparable) {
    MaterialTypeTag tag1{0};
    MaterialTypeTag tag2{0};
    MaterialTypeTag tag3{1};

    EXPECT_EQ(tag1, tag2);
    EXPECT_NE(tag1, tag3);
}

TEST(MaterialTypeTagTest, TagsAreHashable) {
    std::unordered_set<MaterialTypeTag> tags;

    tags.insert(test_tag_1);
    tags.insert(test_tag_2);
    tags.insert(test_tag_1); // Duplicate

    EXPECT_EQ(tags.size(), 2);
    EXPECT_TRUE(tags.contains(test_tag_1));
    EXPECT_TRUE(tags.contains(test_tag_2));
}

TEST(MaterialTypeTagTest, TagIdAccessor) {
    MaterialTypeTag tag{42};
    EXPECT_EQ(tag.id(), 42);
}

TEST(MaterialPriorityTest, ColoredLowerThanTextured) {
    EXPECT_LT(static_cast<int>(colored_material_priority),
              static_cast<int>(textured_material_priority));
}

TEST(MaterialPriorityTest, UserPriorityHighest) {
    EXPECT_GT(static_cast<int>(user_material_priority),
              static_cast<int>(textured_material_priority));
    EXPECT_GT(static_cast<int>(user_material_priority),
              static_cast<int>(colored_material_priority));
}

TEST(MaterialPriorityTest, PrioritiesAreComparable) {
    EXPECT_TRUE(colored_material_priority < textured_material_priority);
    EXPECT_TRUE(textured_material_priority < user_material_priority);
}
