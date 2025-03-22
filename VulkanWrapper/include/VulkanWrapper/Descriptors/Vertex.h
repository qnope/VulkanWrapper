#pragma once

namespace vw {

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

} // namespace vw
