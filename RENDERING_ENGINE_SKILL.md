# Skill: Développement Moteur de Rendu VulkanWrapper

Ce document définit les conventions et patterns à suivre pour développer sur le moteur de rendu VulkanWrapper.

---

## 1. C++23 Moderne

### Concepts

Utiliser les concepts pour contraindre les templates au lieu de SFINAE :

```cpp
// Définition de concept
template <typename T>
concept Vertex =
    std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T> &&
    requires(T x) {
        { T::binding_description(0) } -> std::same_as<vk::VertexInputBindingDescription>;
        { T::attribute_descriptions(0, 0)[0] } -> std::convertible_to<vk::VertexInputAttributeDescription>;
    };

// Utilisation
template <Vertex V>
void add_vertex_binding();
```

### Requires Clauses

Activer conditionnellement les méthodes selon les paramètres template :

```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer {
    // Méthode disponible uniquement si HostVisible == true
    void write(std::span<const T> data, std::size_t offset) requires(HostVisible);

    // Méthode toujours disponible
    [[nodiscard]] vk::DeviceAddress device_address() const;
};
```

### Consteval et Constexpr

Privilégier `consteval` pour les validations compile-time :

```cpp
class Buffer {
    static consteval bool does_support(vk::BufferUsageFlags required) {
        return (flags & required) == required;
    }
};
```

### Structured Bindings

Utiliser systématiquement les structured bindings :

```cpp
// Retour de map::emplace
auto [inserted_it, success] = m_cache.emplace(key, value);

// Itération sur map
for (auto const &[key, value] : m_resources) {
    process(key, value);
}
```

### Opérateur Spaceship

Utiliser `operator<=>` par défaut pour les types comparables :

```cpp
struct ImageKey {
    vk::Format format;
    uint32_t width;
    uint32_t height;

    auto operator<=>(const ImageKey &) const = default;
};
```

### std::optional et std::variant

```cpp
// Optional pour valeurs optionnelles
std::optional<vk::ImageSubresourceRange> range;

// Variant pour types polymorphiques
using ResourceState = std::variant<ImageState, BufferState, AccelerationStructureState>;

// Visitation avec overloaded pattern
std::visit(overloaded{
    [](ImageState const &s) { /* ... */ },
    [](BufferState const &s) { /* ... */ },
    [](AccelerationStructureState const &s) { /* ... */ }
}, state);
```

### std::span pour les Vues

Préférer `std::span` aux pointeurs bruts :

```cpp
void process_vertices(std::span<const Vertex> vertices);
void fill_buffer(std::span<const std::byte> data);
```

---

## 2. Ranges > Boucles

### Règle Fondamentale

**Toujours préférer les algorithmes de ranges aux boucles manuelles.**

### Transformations

```cpp
// ❌ Mauvais : boucle manuelle
std::vector<vk::Image> handles;
for (auto const &img : images) {
    handles.push_back(img->handle());
}

// ✅ Bon : ranges avec transform
auto handles = images
    | std::views::transform(&Image::handle)
    | std::ranges::to<std::vector>();
```

### Recherche

```cpp
// ❌ Mauvais
for (auto const &item : items) {
    if (item.id == target_id) {
        return &item;
    }
}

// ✅ Bon
auto it = std::ranges::find_if(items, [&](auto const &item) {
    return item.id == target_id;
});
```

### Filtrage et Suppression

```cpp
// ❌ Mauvais
for (auto it = items.begin(); it != items.end();) {
    if (should_remove(*it)) {
        it = items.erase(it);
    } else {
        ++it;
    }
}

// ✅ Bon
std::erase_if(items, should_remove);
```

### Copie

```cpp
// ❌ Mauvais
for (size_t i = 0; i < src.size(); ++i) {
    dst[i] = src[i];
}

// ✅ Bon
std::ranges::copy(src, dst.begin());
```

### Tri

```cpp
// ❌ Mauvais
std::sort(items.begin(), items.end(), comparator);

// ✅ Bon
std::ranges::sort(items, comparator);
```

### Views Personnalisées

Définir des adaptateurs réutilisables :

```cpp
// Dans ObjectWithHandle.h
inline constexpr auto to_handle = std::views::transform([](auto const &obj) {
    return obj->handle();
});

// Utilisation
auto vk_images = images | to_handle | std::ranges::to<std::vector>();
```

### Composition de Views

```cpp
auto valid_visible_meshes = meshes
    | std::views::filter(&Mesh::is_valid)
    | std::views::filter(&Mesh::is_visible)
    | std::views::transform(&Mesh::get_data);
```

---

## 3. Vulkan Moderne (1.3 + Ray Tracing)

### Dynamic Rendering (pas de VkRenderPass)

```cpp
vk::RenderingAttachmentInfo color_attachment{
    .imageView = color_view,
    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .clearValue = vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}}
};

vk::RenderingInfo rendering_info{
    .renderArea = {{0, 0}, extent},
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &color_attachment,
    .pDepthAttachment = &depth_attachment
};

cmd.beginRendering(rendering_info);
// draw calls...
cmd.endRendering();
```

### Synchronization2

Utiliser `vk::PipelineStageFlags2` et `vk::AccessFlags2` :

```cpp
vk::ImageMemoryBarrier2 barrier{
    .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
    .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
    .dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead,
    .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    .image = image,
    .subresourceRange = full_range
};

vk::DependencyInfo dep_info{
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &barrier
};

cmd.pipelineBarrier2(dep_info);
```

### Ray Tracing : Structure à Deux Niveaux

```cpp
// BLAS : géométrie par mesh
auto blas = BottomLevelAccelerationStructureBuilder(allocator)
    .add_geometry(vertex_buffer, index_buffer, vertex_count, index_count)
    .build(cmd);

// TLAS : instances de la scène
auto tlas = TopLevelAccelerationStructureBuilder(allocator)
    .add_instance(blas, transform, custom_index, sbt_offset)
    .build(cmd);
```

### Ray Tracing Pipeline

```cpp
auto pipeline = RayTracingPipelineBuilder(device)
    .set_ray_generation_shader(raygen_module)
    .add_miss_shader(miss_module)
    .add_closest_hit_shader(hit_module)
    .set_layout(pipeline_layout)
    .set_max_recursion_depth(2)
    .build();

// Shader Binding Table
ShaderBindingTable sbt(allocator, pipeline);

// Trace rays
cmd.traceRaysKHR(
    sbt.ray_gen_region(),
    sbt.miss_region(),
    sbt.hit_region(),
    sbt.callable_region(),
    width, height, 1
);
```

### Vulkan-Hpp Exclusivement

Utiliser les types `vk::` et jamais les types C `Vk` :

```cpp
// ❌ Mauvais
VkImage image;
VkResult result = vkCreateImage(...);

// ✅ Bon
vk::Image image;
auto [result, created_image] = device.createImage(create_info);
```

---

## 4. ResourceTracker pour Barrières

### Principe

Le `ResourceTracker` gère automatiquement les transitions de layout et les barrières mémoire. **Ne jamais écrire de barrières manuellement.**

### Pattern d'Utilisation

```cpp
vw::Barrier::ResourceTracker tracker;

// 1. Déclarer l'état actuel de la ressource
tracker.track(vw::Barrier::ImageState{
    .image = image->handle(),
    .subresourceRange = image->full_range(),
    .layout = vk::ImageLayout::eUndefined,
    .stage = vk::PipelineStageFlagBits2::eTopOfPipe,
    .access = vk::AccessFlagBits2::eNone
});

// 2. Demander le nouvel état
tracker.request(vw::Barrier::ImageState{
    .image = image->handle(),
    .subresourceRange = image->full_range(),
    .layout = vk::ImageLayout::eColorAttachmentOptimal,
    .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .access = vk::AccessFlagBits2::eColorAttachmentWrite
});

// 3. Générer les barrières minimales
tracker.flush(cmd);  // Émet les vk::pipelineBarrier2() nécessaires
```

### États Supportés

```cpp
// Images
struct ImageState {
    vk::Image image;
    vk::ImageSubresourceRange subresourceRange;
    vk::ImageLayout layout;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

// Buffers
struct BufferState {
    vk::Buffer buffer;
    vk::DeviceSize offset;
    vk::DeviceSize size;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

// Acceleration Structures
struct AccelerationStructureState {
    vk::AccelerationStructureKHR accelerationStructure;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};
```

### Intégration avec Transfer

La classe `Transfer` encapsule un `ResourceTracker` :

```cpp
vw::Transfer transfer;

// Track et request via transfer
transfer.resourceTracker().track(image_state);
transfer.resourceTracker().request(new_state);
transfer.resourceTracker().flush(cmd);

// Opérations haut niveau qui gèrent les transitions
transfer.blit(cmd, src_image, dst_image);
transfer.copy(cmd, staging_buffer, device_buffer);
```

### Tracking Sub-Resources

Le tracker utilise `IntervalSet` pour un tracking fin :

```cpp
// Tracker un seul mip level
tracker.track(vw::Barrier::ImageState{
    .image = image,
    .subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 2,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    },
    .layout = vk::ImageLayout::eTransferDstOptimal,
    // ...
});
```

---

## 5. CMake Moderne (4.x)

### Structure Modulaire

```cmake
# Target principal avec interface propre
add_library(VulkanWrapperCoreLibrary)
add_library(VulkanWrapper::VW ALIAS VulkanWrapperCoreLibrary)

# C++23 obligatoire
target_compile_features(VulkanWrapperCoreLibrary PUBLIC cxx_std_23)

# Headers publics
target_include_directories(VulkanWrapperCoreLibrary PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
```

### Precompiled Headers

```cmake
target_precompile_headers(VulkanWrapperCoreLibrary PUBLIC
    include/VulkanWrapper/3rd_party.h
)
```

### Compilation de Shaders

```cmake
# Fonction personnalisée pour compiler les shaders
function(VwCompileShader TARGET_NAME SOURCE OUTPUT)
    set(extra_args ${ARGN})

    get_filename_component(OUTPUT_DIR ${OUTPUT} DIRECTORY)
    if(OUTPUT_DIR)
        set(MKDIR_CMD ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:${TARGET_NAME}>/${OUTPUT_DIR})
    endif()

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}
        COMMAND ${MKDIR_CMD}
        COMMAND glslangValidator -V ${extra_args}
            ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
            -o $<TARGET_FILE_DIR:${TARGET_NAME}>/${OUTPUT}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}
        COMMENT "Compiling shader ${SOURCE}"
    )

    target_sources(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT})
endfunction()

# Utilisation
VwCompileShader(MyTarget "shaders/main.vert" "main.vert.spv")
VwCompileShader(MyTarget "shaders/raygen.rgen" "raygen.rgen.spv" --target-env vulkan1.2)
```

### Options Conditionnelles

```cmake
option(ENABLE_COVERAGE "Enable code coverage" OFF)

add_library(coverage_options INTERFACE)
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(coverage_options INTERFACE
        -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(coverage_options INTERFACE
        -fprofile-instr-generate -fcoverage-mapping)
endif()
```

### Tests avec GTest

```cmake
include(GoogleTest)

add_executable(MyTests tests/MyTests.cpp)
target_link_libraries(MyTests PRIVATE
    VulkanWrapper::VW
    TestUtils
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(MyTests)
```

---

## 6. Patterns Architecturaux

### Builder Pattern

```cpp
class SamplerBuilder {
public:
    SamplerBuilder &set_filter(vk::Filter filter);
    SamplerBuilder &set_address_mode(vk::SamplerAddressMode mode);
    SamplerBuilder &set_anisotropy(float max_anisotropy);

    [[nodiscard]] std::shared_ptr<Sampler> build() const;

private:
    vk::SamplerCreateInfo m_info{};
};

// Utilisation
auto sampler = SamplerBuilder(device)
    .set_filter(vk::Filter::eLinear)
    .set_address_mode(vk::SamplerAddressMode::eRepeat)
    .set_anisotropy(16.0f)
    .build();
```

### RAII avec Handles Vulkan

```cpp
template <typename HandleType>
class ObjectWithUniqueHandle {
public:
    [[nodiscard]] HandleType handle() const { return *m_handle; }

protected:
    HandleType::CType m_handle;  // vk::UniqueXxx
};
```

### Screen-Space Pass

```cpp
template <typename SlotEnum>
class ScreenSpacePass : public Subpass<SlotEnum> {
protected:
    void render_fullscreen(
        vk::CommandBuffer cmd,
        vk::Extent2D extent,
        vk::RenderingAttachmentInfo color,
        vk::RenderingAttachmentInfo const *depth,
        vk::Pipeline pipeline,
        vk::DescriptorSet descriptor_set,
        void const *push_constants,
        size_t push_constants_size
    );

    [[nodiscard]] std::shared_ptr<Sampler> create_default_sampler();
};
```

### Types Forts pour les Dimensions

```cpp
enum class Width : uint32_t {};
enum class Height : uint32_t {};
enum class Depth : uint32_t {};
enum class MipLevel : uint32_t {};

// Utilisation - impossible de confondre width et height
void create_image(Width w, Height h, Depth d = Depth{1});
```

---

## 7. Gestion d'Erreurs

### Hiérarchie d'Exceptions

```cpp
vw::Exception                    // Base avec std::source_location
├── VulkanException             // Erreurs vk::Result
├── SDLException                // Erreurs SDL
├── VMAException                // Erreurs allocation
├── FileException               // Erreurs I/O
├── AssimpException             // Erreurs chargement modèles
├── ShaderCompilationException  // Erreurs compilation GLSL
└── LogicException              // Erreurs logiques
```

### Fonctions de Vérification

```cpp
// Vulkan result
check_vk(result, "create image");

// SDL
check_sdl(window != nullptr, "create window");

// VMA
check_vma(vmaCreateBuffer(...), "allocate buffer");
```

---

## 8. Conventions de Code

### Nommage

- **Namespaces** : `vw::`, `vw::Barrier::`, `vw::Model::`, `vw::rt::`
- **Types** : `PascalCase` (`Buffer`, `ImageView`)
- **Fonctions/Variables** : `snake_case` (`create_image`, `m_device`)
- **Constantes** : `PascalCase` ou `kConstantName`

### Fichiers

- Headers : `.h` avec `#pragma once`
- Sources : `.cpp`
- Structure miroir `include/` et `src/`

### Formatage (clang-format)

- 80 caractères max par ligne
- 4 espaces d'indentation
- Pointeurs à droite : `int *ptr`, `int &ref`
- Style K&R pour les accolades
