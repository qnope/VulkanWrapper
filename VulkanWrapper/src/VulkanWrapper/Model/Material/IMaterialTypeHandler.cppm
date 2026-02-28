export module vw.model:imaterial_type_handler;
import std3rd;
import vulkan3rd;
import assimp3rd;
import :material_type_tag;
import :material_priority;
import :material;
import vw.vulkan;
import vw.memory;
import vw.sync;
import vw.descriptors;

export namespace vw {} // namespace vw

export namespace vw::Model::Material {

class IMaterialTypeHandler {
  public:
    virtual ~IMaterialTypeHandler() = default;

    [[nodiscard]] const std::filesystem::path &brdf_path() const {
        return m_brdf_path;
    }

    [[nodiscard]] virtual MaterialTypeTag tag() const = 0;
    [[nodiscard]] virtual MaterialPriority priority() const = 0;

    [[nodiscard]] virtual std::optional<Material>
    try_create(const aiMaterial *mat,
               const std::filesystem::path &base_path) = 0;

    [[nodiscard]] virtual vk::DeviceAddress buffer_address() const = 0;
    [[nodiscard]] virtual uint32_t stride() const = 0;

    virtual void upload() = 0;

    [[nodiscard]] virtual std::vector<Barrier::ResourceState>
    get_resources() const = 0;

    [[nodiscard]] virtual std::optional<vk::DescriptorSet>
    additional_descriptor_set() const {
        return std::nullopt;
    }

    [[nodiscard]] virtual std::shared_ptr<const DescriptorSetLayout>
    additional_descriptor_set_layout() const {
        return nullptr;
    }

  protected:
    explicit IMaterialTypeHandler(std::filesystem::path brdf_path)
        : m_brdf_path(std::move(brdf_path)) {}

  private:
    std::filesystem::path m_brdf_path;
};

} // namespace vw::Model::Material
