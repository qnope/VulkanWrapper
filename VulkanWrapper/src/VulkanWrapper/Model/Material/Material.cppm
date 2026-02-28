export module vw.model:material;
import std3rd;
import :material_type_tag;

export namespace vw::Model::Material {

struct Material {
    MaterialTypeTag material_type;
    uint64_t buffer_address;

    Material(MaterialTypeTag type, uint64_t address)
        : material_type{type}
        , buffer_address{address} {}
};

} // namespace vw::Model::Material
