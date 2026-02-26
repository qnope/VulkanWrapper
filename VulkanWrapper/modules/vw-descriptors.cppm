module;
#include "VulkanWrapper/3rd_party.h"

export module vw:descriptors;
import "VulkanWrapper/vw_vulkan.h";
import :core;
import :utils;
import :vulkan;
import :synchronization;
import :image;
import :memory;

export namespace vw {

// ---- Vertex ----

template <typename T> struct format_from;

template <>
struct format_from<float>
    : std::integral_constant<vk::Format, vk::Format::eR32Sfloat> {};

template <>
struct format_from<glm::vec2>
    : std::integral_constant<vk::Format, vk::Format::eR32G32Sfloat> {};

template <>
struct format_from<glm::vec3>
    : std::integral_constant<vk::Format, vk::Format::eR32G32B32Sfloat> {};

template <>
struct format_from<glm::vec4>
    : std::integral_constant<vk::Format, vk::Format::eR32G32B32A32Sfloat> {};

template <typename... Ts> class VertexInterface {
  public:
    [[nodiscard]] static constexpr auto binding_description(int binding) {
        return vk::VertexInputBindingDescription(binding, (sizeof(Ts) + ...),
                                                 vk::VertexInputRate::eVertex);
    }

    [[nodiscard]] static constexpr auto attribute_descriptions(int binding,
                                                               int location) {
        constexpr auto offsets = []() {
            auto sizes = std::array{sizeof(Ts)...};
            std::exclusive_scan(begin(sizes), end(sizes), begin(sizes), 0);
            return sizes;
        }();

        std::array attributes = {
            vk::VertexInputAttributeDescription().setFormat(
                format_from<Ts>::value)...};

        for (int i = 0; i < sizeof...(Ts); ++i) {
            attributes[i].location = location + i;
            attributes[i].binding = binding;
            attributes[i].offset = offsets[i];
        }

        return attributes;
    }
};

template <typename Position>
struct ColoredVertex : VertexInterface<Position, glm::vec3> {
    ColoredVertex() noexcept = default;
    ColoredVertex(Position position, glm::vec3 color) noexcept
        : position{position}
        , color{color} {}

    Position position{};
    glm::vec3 color{};
};

template <typename Position>
struct ColoredAndTexturedVertex
    : VertexInterface<Position, glm::vec3, glm::vec2> {
    ColoredAndTexturedVertex() noexcept = default;
    ColoredAndTexturedVertex(Position position, glm::vec3 color,
                             glm::vec2 texCoord) noexcept
        : position{position}
        , color{color}
        , texCoord{texCoord} {}

    Position position{};
    glm::vec3 color{};
    glm::vec2 texCoord{};
};

using ColoredVertex2D = ColoredVertex<glm::vec2>;
using ColoredAndTexturedVertex2D = ColoredAndTexturedVertex<glm::vec2>;

using ColoredVertex3D = ColoredVertex<glm::vec3>;
using ColoredAndTexturedVertex3D = ColoredAndTexturedVertex<glm::vec3>;

struct FullVertex3D
    : VertexInterface<glm::vec3, glm::vec3, glm::vec3, glm::vec3, glm::vec2> {
    FullVertex3D() = default;

    FullVertex3D(glm::vec3 position, glm::vec3 normal, glm::vec3 tangeant,
                 glm::vec3 bitangeant)
        : position{position}
        , normal{normal}
        , tangeant{tangeant}
        , bitangeant{bitangeant} {}

    FullVertex3D(glm::vec3 position, glm::vec3 normal, glm::vec3 tangeant,
                 glm::vec3 bitangeant, glm::vec2 uv)
        : position{position}
        , normal{normal}
        , tangeant{tangeant}
        , bitangeant{bitangeant}
        , uv{uv} {}

    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec3 tangeant{};
    glm::vec3 bitangeant{};
    glm::vec2 uv{};
};

struct Vertex3D : VertexInterface<glm::vec3> {
    Vertex3D() = default;

    Vertex3D(glm::vec3 position) noexcept
        : position{position} {}

    glm::vec3 position;
};

template <typename T>
concept Vertex =
    std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T> &&
    requires(T x) {
        {
            T::binding_description(0)
        } -> std::same_as<vk::VertexInputBindingDescription>;
        {
            T::attribute_descriptions(0, 0)[0]
        } -> std::convertible_to<vk::VertexInputAttributeDescription>;
    };

// ---- DescriptorSet ----

class DescriptorSet : public ObjectWithHandle<vk::DescriptorSet> {
  public:
    DescriptorSet(vk::DescriptorSet handle,
                  std::vector<Barrier::ResourceState> resources) noexcept;

    const std::vector<Barrier::ResourceState> &resources() const noexcept;

  private:
    std::vector<Barrier::ResourceState> m_resources;
};

// ---- DescriptorSetLayout ----

class DescriptorSetLayout
    : public ObjectWithUniqueHandle<vk::UniqueDescriptorSetLayout> {
  public:
    DescriptorSetLayout(std::vector<vk::DescriptorSetLayoutBinding> pool_sizes,
                        vk::UniqueDescriptorSetLayout set_layout);

    [[nodiscard]] std::vector<vk::DescriptorPoolSize> get_pool_sizes() const;

  private:
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
};

class DescriptorSetLayoutBuilder {
  public:
    DescriptorSetLayoutBuilder(std::shared_ptr<const Device> device);

    DescriptorSetLayoutBuilder &with_uniform_buffer(vk::ShaderStageFlags stages,
                                                    int number);
    DescriptorSetLayoutBuilder &with_sampled_image(vk::ShaderStageFlags stages,
                                                   int number);
    DescriptorSetLayoutBuilder &with_combined_image(vk::ShaderStageFlags stages,
                                                    int number);
    DescriptorSetLayoutBuilder &
    with_input_attachment(vk::ShaderStageFlags stages);
    DescriptorSetLayoutBuilder &
    with_acceleration_structure(vk::ShaderStageFlags stages);
    DescriptorSetLayoutBuilder &with_storage_image(vk::ShaderStageFlags stages,
                                                   int number);
    DescriptorSetLayoutBuilder &with_storage_buffer(vk::ShaderStageFlags stages,
                                                    int number = 1);
    DescriptorSetLayoutBuilder &with_sampler(vk::ShaderStageFlags stages);
    DescriptorSetLayoutBuilder &
    with_sampled_images_bindless(vk::ShaderStageFlags stages,
                                 uint32_t max_count);
    DescriptorSetLayoutBuilder &
    with_storage_buffer_bindless(vk::ShaderStageFlags stages);

    std::shared_ptr<DescriptorSetLayout> build();

  private:
    std::shared_ptr<const Device> m_device;
    uint32_t m_current_binding = 0;
    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
    std::vector<vk::DescriptorBindingFlags> m_binding_flags;
    bool m_has_bindless = false;
};

// ---- DescriptorAllocator ----

class DescriptorAllocator {
  public:
    DescriptorAllocator();

    void add_uniform_buffer(int binding, vk::Buffer buffer,
                            vk::DeviceSize offset, vk::DeviceSize size,
                            vk::PipelineStageFlags2 stage,
                            vk::AccessFlags2 access,
                            uint32_t array_element = 0);

    void add_storage_buffer(int binding, vk::Buffer buffer,
                            vk::DeviceSize offset, vk::DeviceSize size,
                            vk::PipelineStageFlags2 stage,
                            vk::AccessFlags2 access,
                            uint32_t array_element = 0);

    void add_combined_image(int binding, const CombinedImage &image,
                            vk::PipelineStageFlags2 stage,
                            vk::AccessFlags2 access,
                            uint32_t array_element = 0);

    void add_storage_image(int binding, const ImageView &image_view,
                           vk::PipelineStageFlags2 stage,
                           vk::AccessFlags2 access, uint32_t array_element = 0);

    void add_input_attachment(int binding,
                              std::shared_ptr<const ImageView> image_view,
                              vk::PipelineStageFlags2 stage,
                              vk::AccessFlags2 access,
                              uint32_t array_element = 0);

    void add_acceleration_structure(int binding,
                                    vk::AccelerationStructureKHR tlas,
                                    vk::PipelineStageFlags2 stage,
                                    vk::AccessFlags2 access,
                                    uint32_t array_element = 0);

    void add_sampler(int binding, vk::Sampler sampler,
                     uint32_t array_element = 0);

    void add_sampled_image(int binding, const ImageView &image_view,
                           vk::PipelineStageFlags2 stage,
                           vk::AccessFlags2 access, uint32_t array_element = 0);

    void add_sampled_image(int binding,
                           std::shared_ptr<const ImageView> image_view,
                           vk::PipelineStageFlags2 stage,
                           vk::AccessFlags2 access, uint32_t array_element = 0);

    [[nodiscard]] std::vector<vk::WriteDescriptorSet>
    get_write_descriptors() const;

    [[nodiscard]] std::vector<Barrier::ResourceState> get_resources() const;

    bool operator==(const DescriptorAllocator &) const = default;

  private:
    struct BufferUpdate {
        vk::DescriptorBufferInfo info;
        vk::WriteDescriptorSet write;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
        bool operator==(const BufferUpdate &) const = default;
    };

    struct ImageUpdate {
        vk::DescriptorImageInfo info;
        vk::WriteDescriptorSet write;
        vk::Image image;
        vk::ImageSubresourceRange subresource_range;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
        bool operator==(const ImageUpdate &) const = default;
    };

    struct AccelerationStructureUpdate {
        vk::AccelerationStructureKHR accelerationStructure;
        vk::WriteDescriptorSetAccelerationStructureKHR info;
        vk::WriteDescriptorSet write;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
        bool operator==(const AccelerationStructureUpdate &) const = default;
    };

    struct SamplerUpdate {
        vk::DescriptorImageInfo info;
        vk::WriteDescriptorSet write;
        bool operator==(const SamplerUpdate &) const = default;
    };

    std::vector<BufferUpdate> m_bufferUpdate;
    std::vector<ImageUpdate> m_imageUpdate;
    std::vector<SamplerUpdate> m_samplerUpdate;
    std::optional<AccelerationStructureUpdate> m_accelerationStructureUpdate;
};

} // namespace vw

// std::hash specialization for DescriptorAllocator
// Must appear before unordered_map<DescriptorAllocator, ...> usage
template <> struct std::hash<vw::DescriptorAllocator> {
    std::size_t
    operator()(const vw::DescriptorAllocator &allocator) const noexcept {
        return 0;
    }
};

export namespace vw {

// ---- DescriptorPool ----

namespace Internal {
class DescriptorPoolImpl
    : public ObjectWithUniqueHandle<vk::UniqueDescriptorPool> {
  public:
    DescriptorPoolImpl(
        vk::UniqueDescriptorPool pool, std::shared_ptr<const Device> device,
        const std::shared_ptr<const DescriptorSetLayout> &layout);

    std::optional<vk::DescriptorSet> allocate_set();

  private:
    uint32_t m_number_allocation = 0;
    std::vector<vk::DescriptorSet> m_sets;
};
} // namespace Internal

class DescriptorPool {
  public:
    DescriptorPool(std::shared_ptr<const Device> device,
                   std::shared_ptr<const DescriptorSetLayout> layout,
                   bool update_after_bind = false);

    [[nodiscard]] std::shared_ptr<const DescriptorSetLayout>
    layout() const noexcept;

    [[nodiscard]] DescriptorSet
    allocate_set(const DescriptorAllocator &descriptorAllocator) noexcept;

    [[nodiscard]] DescriptorSet allocate_set();

    void update_set(vk::DescriptorSet set,
                    const DescriptorAllocator &allocator);

  private:
    vk::DescriptorSet allocate_descriptor_set_from_last_pool();

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const DescriptorSetLayout> m_layout;
    std::vector<Internal::DescriptorPoolImpl> m_descriptor_pools;
    std::unordered_map<DescriptorAllocator, DescriptorSet> m_sets;
    bool m_update_after_bind = false;
};

class DescriptorPoolBuilder {
  public:
    DescriptorPoolBuilder(
        std::shared_ptr<const Device> device,
        const std::shared_ptr<const DescriptorSetLayout> &layout);

    DescriptorPoolBuilder &with_update_after_bind();

    DescriptorPool build();

  private:
    std::shared_ptr<const Device> m_device;
    std::shared_ptr<const DescriptorSetLayout> m_layout;
    bool m_update_after_bind = false;
};

} // namespace vw
