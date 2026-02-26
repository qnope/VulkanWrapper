module;
#include "VulkanWrapper/3rd_party.h"
#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <ranges>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

struct aiMaterial;
struct aiMesh;

export module vw:model;
import :core;
import :utils;
import :vulkan;
import :synchronization;
import :image;
import :memory;
import :descriptors;
import :pipeline;

export namespace vw::Model {

// Forward declarations
class MeshManager;

namespace Material {

// MaterialTypeTag and MaterialTypeId are defined in 3rd_party.h
// (global module fragment) because the VW_DEFINE_MATERIAL_TYPE macro needs
// them. Re-export from the module:
using ::vw::Model::Material::MaterialTypeId;
using ::vw::Model::Material::MaterialTypeTag;

// ---- Material ----

struct Material {
    MaterialTypeTag material_type;
    uint64_t buffer_address;

    Material(MaterialTypeTag type, uint64_t address)
        : material_type{type}
        , buffer_address{address} {}
};

// ---- MaterialPriority ----

enum class MaterialPriority {};

constexpr MaterialPriority user_material_priority = MaterialPriority{1024};
constexpr MaterialPriority colored_material_priority = MaterialPriority{0};
constexpr MaterialPriority textured_material_priority = MaterialPriority{1};
constexpr MaterialPriority emissive_textured_material_priority =
    MaterialPriority{2};

// ---- MaterialData ----

struct TexturedMaterialData {
    uint32_t diffuse_texture_index;
};
static_assert(sizeof(TexturedMaterialData) == 4);

struct ColoredMaterialData {
    glm::vec3 color;
};
static_assert(sizeof(ColoredMaterialData) == 12);

struct EmissiveTexturedMaterialData {
    uint32_t diffuse_texture_index;
    float intensity;
};
static_assert(sizeof(EmissiveTexturedMaterialData) == 8);

// ---- IMaterialTypeHandler ----

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

// ---- MaterialTypeHandler<GpuData> ----

template <typename GpuData>
class MaterialTypeHandler : public IMaterialTypeHandler {
  public:
    using SSBOType = Buffer<GpuData, true, StorageBufferUsage>;

    template <typename Derived, typename... Args>
    static std::unique_ptr<Derived> create(std::shared_ptr<const Device> device,
                                           std::shared_ptr<Allocator> allocator,
                                           Args &&...args) {
        static_assert(std::is_base_of_v<MaterialTypeHandler, Derived>,
                      "Derived must inherit from MaterialTypeHandler");
        return std::unique_ptr<Derived>(
            new Derived(std::move(device), std::move(allocator),
                        std::forward<Args>(args)...));
    }

    [[nodiscard]] std::optional<Material>
    try_create(const aiMaterial *mat,
               const std::filesystem::path &base_path) final {
        auto data = try_create_gpu_data(mat, base_path);
        if (!data) {
            return std::nullopt;
        }
        uint32_t index = static_cast<uint32_t>(m_material_data.size());
        m_material_data.push_back(*data);
        m_dirty = true;
        return Material{tag(),
                        m_ssbo.device_address() +
                            index * sizeof(GpuData)};
    }

    [[nodiscard]] Material create_material(GpuData data) {
        uint32_t index = static_cast<uint32_t>(m_material_data.size());
        m_material_data.push_back(std::move(data));
        m_dirty = true;
        return Material{tag(),
                        m_ssbo.device_address() +
                            index * sizeof(GpuData)};
    }

    [[nodiscard]] vk::DeviceAddress buffer_address() const final {
        return m_ssbo.device_address();
    }

    [[nodiscard]] uint32_t stride() const final { return sizeof(GpuData); }

    void upload() final {
        if (!m_dirty || m_material_data.empty()) {
            return;
        }
        m_ssbo.write(m_material_data, 0);
        m_dirty = false;
    }

    [[nodiscard]] std::vector<Barrier::ResourceState>
    get_resources() const final {
        std::vector<Barrier::ResourceState> resources;

        resources.push_back(Barrier::BufferState{
            .buffer = m_ssbo.handle(),
            .offset = 0,
            .size = m_ssbo.size_bytes(),
            .stage = vk::PipelineStageFlagBits2::eVertexShader |
                     vk::PipelineStageFlagBits2::eFragmentShader |
                     vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
            .access = vk::AccessFlagBits2::eShaderStorageRead});

        auto texture_resources = get_texture_resources();
        resources.insert(resources.end(), texture_resources.begin(),
                         texture_resources.end());

        return resources;
    }

  protected:
    MaterialTypeHandler(std::shared_ptr<const Device> device,
                        std::shared_ptr<Allocator> allocator,
                        std::filesystem::path brdf_path)
        : IMaterialTypeHandler(std::move(brdf_path))
        , m_device{std::move(device)}
        , m_allocator{std::move(allocator)}
        , m_ssbo{create_buffer<SSBOType>(*m_allocator, 1024)} {}

    [[nodiscard]] virtual std::optional<GpuData>
    try_create_gpu_data(const aiMaterial *mat,
                        const std::filesystem::path &base_path) = 0;

    [[nodiscard]] virtual std::vector<Barrier::ResourceState>
    get_texture_resources() const {
        return {};
    }

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<Allocator> m_allocator;
    SSBOType m_ssbo;

    std::vector<GpuData> m_material_data;
    bool m_dirty = false;
};

// ---- BindlessTextureManager ----

class BindlessTextureManager {
  public:
    static constexpr uint32_t MAX_TEXTURES = 4096;

    BindlessTextureManager(std::shared_ptr<const Device> device,
                           std::shared_ptr<Allocator> allocator,
                           std::shared_ptr<StagingBufferManager> staging);

    uint32_t register_texture(const std::filesystem::path &path);

    [[nodiscard]] vk::DescriptorSet descriptor_set() const noexcept;

    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    layout() const noexcept;

    [[nodiscard]] uint32_t texture_count() const noexcept;

    void update_descriptors();

    [[nodiscard]] std::vector<Barrier::ResourceState> get_resources() const;

    [[nodiscard]] vk::Sampler sampler() const noexcept;

    void write_image_descriptors(vk::DescriptorSet dest_set,
                                 uint32_t dest_binding) const;

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<Allocator> m_allocator;
    std::shared_ptr<StagingBufferManager> m_staging;

    std::shared_ptr<DescriptorSetLayout> m_layout;
    DescriptorPool m_pool;
    DescriptorSet m_descriptor_set;
    std::shared_ptr<const Sampler> m_sampler;

    std::vector<CombinedImage> m_combined_images;
    std::unordered_map<std::filesystem::path, uint32_t> m_path_to_index;

    uint32_t m_last_updated_count = 0;
};

// ---- ColoredMaterialHandler ----

VW_DECLARE_MATERIAL_TYPE(colored_material_tag);

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

// ---- TexturedMaterialHandler ----

VW_DECLARE_MATERIAL_TYPE(textured_material_tag);

class TexturedMaterialHandler
    : public MaterialTypeHandler<TexturedMaterialData> {
    friend class MaterialTypeHandler<TexturedMaterialData>;

  public:
    using Base = MaterialTypeHandler<TexturedMaterialData>;

    [[nodiscard]] MaterialTypeTag tag() const override;
    [[nodiscard]] MaterialPriority priority() const override;

    [[nodiscard]] std::optional<vk::DescriptorSet>
    additional_descriptor_set() const override;

    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    additional_descriptor_set_layout() const override;

    [[nodiscard]] Material
    create_material(const std::filesystem::path &texture_path);

  protected:
    [[nodiscard]] std::optional<TexturedMaterialData>
    try_create_gpu_data(const aiMaterial *mat,
                        const std::filesystem::path &base_path) override;

    [[nodiscard]] std::vector<Barrier::ResourceState>
    get_texture_resources() const override;

  private:
    TexturedMaterialHandler(std::shared_ptr<const Device> device,
                            std::shared_ptr<Allocator> allocator,
                            BindlessTextureManager &texture_manager);

    BindlessTextureManager &m_texture_manager;
};

// ---- EmissiveTexturedMaterialHandler ----

VW_DECLARE_MATERIAL_TYPE(emissive_textured_material_tag);

class EmissiveTexturedMaterialHandler
    : public MaterialTypeHandler<EmissiveTexturedMaterialData> {
    friend class MaterialTypeHandler<EmissiveTexturedMaterialData>;

  public:
    using Base = MaterialTypeHandler<EmissiveTexturedMaterialData>;

    [[nodiscard]] MaterialTypeTag tag() const override;
    [[nodiscard]] MaterialPriority priority() const override;

    [[nodiscard]] std::optional<vk::DescriptorSet>
    additional_descriptor_set() const override;

    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    additional_descriptor_set_layout() const override;

    [[nodiscard]] Material
    create_material(const std::filesystem::path &texture_path,
                    float intensity);

  protected:
    [[nodiscard]] std::optional<EmissiveTexturedMaterialData>
    try_create_gpu_data(const aiMaterial *mat,
                        const std::filesystem::path &base_path) override;

    [[nodiscard]] std::vector<Barrier::ResourceState>
    get_texture_resources() const override;

  private:
    EmissiveTexturedMaterialHandler(
        std::shared_ptr<const Device> device,
        std::shared_ptr<Allocator> allocator,
        BindlessTextureManager &texture_manager);

    BindlessTextureManager &m_texture_manager;
};

// ---- BindlessMaterialManager ----

class BindlessMaterialManager {
  public:
    BindlessMaterialManager(std::shared_ptr<const Device> device,
                            std::shared_ptr<Allocator> allocator,
                            std::shared_ptr<StagingBufferManager> staging);

    template <typename Handler, typename... Args>
    void register_handler(Args &&...args) {
        auto handler = Handler::Base::template create<Handler>(
            m_device, m_allocator, std::forward<Args>(args)...);
        auto tag = handler->tag();
        m_handlers[tag] = std::move(handler);
        m_sorted_handlers.clear();
    }

    [[nodiscard]] Material
    create_material(const aiMaterial *mat,
                    const std::filesystem::path &base_path);

    [[nodiscard]] BindlessTextureManager &texture_manager() noexcept;
    [[nodiscard]] const BindlessTextureManager &
    texture_manager() const noexcept;

    [[nodiscard]] IMaterialTypeHandler *handler(MaterialTypeTag tag);
    [[nodiscard]] const IMaterialTypeHandler *
    handler(MaterialTypeTag tag) const;

    void upload_all();

    [[nodiscard]] auto handlers() const {
        return m_handlers
               | std::views::transform([](const auto &pair)
                     -> std::pair<MaterialTypeTag,
                                  const IMaterialTypeHandler *> {
                   return {pair.first, pair.second.get()};
               });
    }

    [[nodiscard]] std::vector<
        std::pair<MaterialTypeTag, const IMaterialTypeHandler *>>
    ordered_handlers() const {
        auto result = handlers() | std::ranges::to<std::vector>();
        std::ranges::sort(result, [](const auto &a, const auto &b) {
            return a.first < b.first;
        });
        return result;
    }

    [[nodiscard]] std::unordered_map<MaterialTypeTag, uint32_t>
    sbt_mapping() const {
        std::unordered_map<MaterialTypeTag, uint32_t> mapping;
        uint32_t index = 0;
        for (auto [tag, _] : ordered_handlers()) {
            mapping[tag] = index++;
        }
        return mapping;
    }

    [[nodiscard]] std::vector<Barrier::ResourceState> get_resources() const;

  private:
    void ensure_sorted_handlers() const;

    std::shared_ptr<const Device> m_device;
    std::shared_ptr<Allocator> m_allocator;
    std::shared_ptr<StagingBufferManager> m_staging;

    BindlessTextureManager m_texture_manager;

    std::unordered_map<MaterialTypeTag, std::unique_ptr<IMaterialTypeHandler>>
        m_handlers;
    mutable std::vector<IMaterialTypeHandler *> m_sorted_handlers;
};

} // namespace Material

// ---- Mesh type aliases ----

using Vertex3DBuffer = Buffer<Vertex3D, false, VertexBufferUsage>;
using FullVertex3DBuffer = Buffer<FullVertex3D, false, VertexBufferUsage>;

// ---- MeshPushConstants ----

struct MeshPushConstants {
    glm::mat4 transform;
    vk::DeviceAddress material_address;
};

// ---- Mesh ----

class Mesh {
  public:
    Mesh(std::shared_ptr<const Vertex3DBuffer> vertex_buffer,
         std::shared_ptr<const FullVertex3DBuffer> full_vertex_buffer,
         std::shared_ptr<const IndexBuffer> index_buffer,
         Material::Material material, uint32_t indice_count, int vertex_offset,
         int first_index, int vertices_count);

    [[nodiscard]] Material::MaterialTypeTag material_type_tag() const noexcept;

    void draw(vk::CommandBuffer cmd_buffer,
              const PipelineLayout &pipeline_layout,
              const glm::mat4 &transform) const;

    void draw_zpass(vk::CommandBuffer cmd_buffer,
                    const PipelineLayout &pipeline_layout,
                    const glm::mat4 &transform) const;

    [[nodiscard]] vk::AccelerationStructureGeometryKHR
    acceleration_structure_geometry() const noexcept;

    [[nodiscard]] vk::AccelerationStructureBuildRangeInfoKHR
    acceleration_structure_range_info() const noexcept;

    [[nodiscard]] uint32_t index_count() const noexcept {
        return m_indice_count;
    }

    [[nodiscard]] const Material::Material &material() const noexcept {
        return m_material;
    }

    [[nodiscard]] std::shared_ptr<const FullVertex3DBuffer>
    full_vertex_buffer() const noexcept {
        return m_full_vertex_buffer;
    }

    [[nodiscard]] std::shared_ptr<const IndexBuffer>
    index_buffer() const noexcept {
        return m_index_buffer;
    }

    [[nodiscard]] int vertex_offset() const noexcept { return m_vertex_offset; }

    [[nodiscard]] int first_index() const noexcept { return m_first_index; }

    [[nodiscard]] size_t geometry_hash() const noexcept;

    bool operator==(const Mesh &other) const noexcept;

  private:
    std::shared_ptr<const Vertex3DBuffer> m_vertex_buffer;
    std::shared_ptr<const FullVertex3DBuffer> m_full_vertex_buffer;
    std::shared_ptr<const IndexBuffer> m_index_buffer;
    Material::Material m_material;

    uint32_t m_indice_count;
    int m_vertex_offset;
    int m_first_index;
    int m_vertices_count;
};

// ---- MeshInstance / Scene ----

struct MeshInstance {
    Mesh mesh;
    glm::mat4 transform = glm::mat4(1.0f);
};

class Scene {
  public:
    Scene() = default;

    void add_mesh_instance(const Mesh &mesh) {
        m_instances.push_back(MeshInstance{mesh, glm::mat4(1.0f)});
    }

    void add_mesh_instance(const Mesh &mesh, const glm::mat4 &transform) {
        m_instances.push_back(MeshInstance{mesh, transform});
    }

    [[nodiscard]] const std::vector<MeshInstance> &instances() const noexcept {
        return m_instances;
    }

    [[nodiscard]] std::vector<MeshInstance> &instances() noexcept {
        return m_instances;
    }

    void clear() noexcept { m_instances.clear(); }

    [[nodiscard]] size_t size() const noexcept { return m_instances.size(); }

  private:
    std::vector<MeshInstance> m_instances;
};

// ---- MeshManager ----

class MeshManager {
  public:
    MeshManager(std::shared_ptr<const Device> device,
                std::shared_ptr<Allocator> allocator);

    void add_mesh(std::vector<FullVertex3D> vertices,
                  std::vector<uint32_t> indices,
                  Material::Material material);

    void read_file(const std::filesystem::path &path);

    [[nodiscard]] vk::CommandBuffer fill_command_buffer();

    [[nodiscard]] const std::vector<Mesh> &meshes() const noexcept;

    [[nodiscard]] Material::BindlessMaterialManager &
    material_manager() noexcept;

    [[nodiscard]] const Material::BindlessMaterialManager &
    material_manager() const noexcept;

  private:
    std::shared_ptr<StagingBufferManager> m_staging_buffer_manager;
    BufferList<Vertex3D, false, VertexBufferUsage> m_vertex_buffer;
    BufferList<FullVertex3D, false, VertexBufferUsage> m_full_vertex_buffer;
    IndexBufferList m_index_buffer;
    Material::BindlessMaterialManager m_material_manager;
    std::vector<Mesh> m_meshes;
};

// ---- Importer ----

void import_model(const std::filesystem::path &path, MeshManager &mesh_manager);

// ---- Internal ----

namespace Internal {

struct MaterialInfo {
    MaterialInfo(const aiMaterial *material,
                 const std::filesystem::path &directory_path);

    std::optional<std::filesystem::path> diffuse_texture_path;
    std::optional<glm::vec4> diffuse_color;

  private:
    void decode_diffuse_texture(const aiMaterial *material,
                                const std::filesystem::path &directory_path);
    void decode_diffuse_color(const aiMaterial *material);
};

struct MeshInfo {
    MeshInfo(const aiMesh *mesh);

    std::vector<Vertex3D> vertices;
    std::vector<FullVertex3D> full_vertices;
    std::vector<uint32_t> indices;
    uint32_t material_index;
};

} // namespace Internal

} // namespace vw::Model

// ---- MeshRenderer ----
// Lives in :model (not :pipeline) because it depends on Model::Mesh and
// Model::Material::MaterialTypeTag.

export namespace vw {

class MeshRenderer {
  public:
    void add_pipeline(Model::Material::MaterialTypeTag tag,
                      std::shared_ptr<const Pipeline> pipeline);

    void draw_mesh(vk::CommandBuffer cmd_buffer, const Model::Mesh &mesh,
                   const glm::mat4 &transform) const;

    [[nodiscard]] std::shared_ptr<const Pipeline>
    pipeline_for(Model::Material::MaterialTypeTag tag) const;

  private:
    std::unordered_map<Model::Material::MaterialTypeTag,
                       std::shared_ptr<const Pipeline>>
        m_pipelines;
};

} // namespace vw

// std::hash<MaterialTypeTag> is in 3rd_party.h (global module fragment)

template <> struct std::hash<vw::Model::Mesh> {
    size_t operator()(const vw::Model::Mesh &mesh) const noexcept {
        return mesh.geometry_hash();
    }
};
