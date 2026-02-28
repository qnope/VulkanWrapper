export module vw.model:colored_material_handler;
import std3rd;
import :material_type_tag;
import :material_priority;
import :material;
import :material_data;
import :material_type_handler;
import vw.vulkan;
import vw.memory;

export namespace vw::Model::Material {

extern const MaterialTypeTag colored_material_tag;

class ColoredMaterialHandler : public MaterialTypeHandler<ColoredMaterialData> {
    friend class MaterialTypeHandler<ColoredMaterialData>;

  public:
    using Base = MaterialTypeHandler<ColoredMaterialData>;

    [[nodiscard]] MaterialTypeTag tag() const override;
    [[nodiscard]] MaterialPriority priority() const override;

  protected:
    [[nodiscard]] std::optional<ColoredMaterialData>
    try_create_gpu_data(const aiMaterial *mat,
                        const std::filesystem::path &base_path) override;

  private:
    ColoredMaterialHandler(std::shared_ptr<const Device> device,
                           std::shared_ptr<Allocator> allocator);
};

} // namespace vw::Model::Material
