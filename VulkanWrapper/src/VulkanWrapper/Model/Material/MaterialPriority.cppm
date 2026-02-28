export module vw.model:material_priority;

export namespace vw::Model::Material {

enum class MaterialPriority {};

constexpr MaterialPriority user_material_priority = MaterialPriority{1024};
constexpr MaterialPriority colored_material_priority = MaterialPriority{0};
constexpr MaterialPriority textured_material_priority = MaterialPriority{1};
constexpr MaterialPriority emissive_textured_material_priority =
    MaterialPriority{2};

} // namespace vw::Model::Material
