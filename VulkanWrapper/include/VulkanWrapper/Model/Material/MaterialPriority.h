#pragma once

namespace vw::Model::Material {
enum class MaterialPriority {};

constexpr MaterialPriority user_material_priority = MaterialPriority{1024};
constexpr MaterialPriority colored_material_priority = MaterialPriority{0};
constexpr MaterialPriority textured_material_priority = MaterialPriority{1};
} // namespace vw::Model::Material
