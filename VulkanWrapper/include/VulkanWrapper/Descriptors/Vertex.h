#pragma once

namespace vw {
struct ColoredVertex2D {
    glm::vec2 vertex;
    glm::vec3 color;

    [[nodiscard]] static constexpr auto binding_description(int binding) {
        return vk::VertexInputBindingDescription(
            binding, sizeof(ColoredVertex2D), vk::VertexInputRate::eVertex);
    }

    [[nodiscard]] static constexpr auto attribute_descriptions(int binding,
                                                               int location) {
        return std::array{vk::VertexInputAttributeDescription(
                              location + 0, binding, vk::Format::eR32G32Sfloat,
                              offsetof(ColoredVertex2D, vertex)),
                          vk::VertexInputAttributeDescription(
                              location + 1, binding,
                              vk::Format::eR32G32B32Sfloat,
                              offsetof(ColoredVertex2D, color))};
    }
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

} // namespace vw
