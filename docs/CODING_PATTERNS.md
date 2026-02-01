# Guide des Bonnes Pratiques - VulkanWrapper

Ce document résume les patterns et conventions de codage utilisés dans ce projet.

## Table des matières

1. [Builder Pattern](#1-builder-pattern)
2. [RAII et Gestion de la Mémoire](#2-raii-et-gestion-de-la-mémoire)
3. [Buffers Type-Safe](#3-buffers-type-safe)
4. [Gestion des Erreurs](#4-gestion-des-erreurs)
5. [Types Forts (Strong Types)](#5-types-forts-strong-types)
6. [Conventions de Nommage](#6-conventions-de-nommage)
7. [Fonctionnalités C++23](#7-fonctionnalités-c23)

---

## 1. Builder Pattern

Les objets complexes utilisent le pattern Builder avec une API fluide.

### Bon exemple

```cpp
// Construction fluide et lisible
auto instance = InstanceBuilder()
    .addExtension(VK_KHR_SURFACE_EXTENSION_NAME)
    .setDebug()
    .setApiVersion(ApiVersion::e13)
    .build();

auto device = instance->findGpu()
    .with_queue(vk::QueueFlagBits::eGraphics)
    .with_dynamic_rendering()
    .with_ray_tracing()
    .build();
```

### Mauvais exemple

```cpp
// Constructeur avec trop de paramètres - difficile à lire et maintenir
auto instance = Instance(
    {VK_KHR_SURFACE_EXTENSION_NAME},  // extensions
    {},                                // layers
    true,                              // debug
    ApiVersion::e13,                   // version
    vk::InstanceCreateFlags{}          // flags
);

// Ordre des paramètres non évident, risque d'erreur
auto device = Device(
    physicalDevice,
    vk::QueueFlagBits::eGraphics,
    true,   // dynamic rendering?
    true,   // ray tracing?
    false   // presentation?
);
```

### Structure d'un Builder

```cpp
class GraphicsPipelineBuilder {
  public:
    // Chaque méthode retourne une référence pour le chaînage
    GraphicsPipelineBuilder &add_shader(vk::ShaderStageFlagBits flags,
                                        std::shared_ptr<const ShaderModule> module);
    GraphicsPipelineBuilder &add_dynamic_state(vk::DynamicState state);
    GraphicsPipelineBuilder &with_fixed_viewport(int width, int height);

    // build() construit l'objet final
    std::shared_ptr<const Pipeline> build();

  private:
    // État interne accumulé
    std::vector<ShaderStageInfo> m_shaders;
    std::vector<vk::DynamicState> m_dynamic_states;
    // ...
};
```

---

## 2. RAII et Gestion de la Mémoire

### Classes de base pour les handles Vulkan

```cpp
// Pour les handles uniques (destruction automatique)
template <typename UniqueHandle>
using ObjectWithUniqueHandle = ObjectWithHandle<UniqueHandle>;

class Pipeline : public ObjectWithUniqueHandle<vk::UniquePipeline> {
    // Le destructeur de vk::UniquePipeline détruit automatiquement le pipeline
};

// Pour les handles partagés
class Instance : public ObjectWithHandle<std::shared_ptr<Impl>> {
    // Destruction quand le dernier shared_ptr est détruit
};
```

### Bon exemple - RAII pour l'enregistrement

```cpp
// CommandBufferRecorder: begin() dans le constructeur, end() dans le destructeur
{
    CommandBufferRecorder recorder(commandBuffer);
    // Enregistrement des commandes...
    recorder.bindPipeline(pipeline);
    recorder.draw(3, 1, 0, 0);
} // end() appelé automatiquement ici

// Non-copiable, non-movable pour éviter les erreurs
CommandBufferRecorder(CommandBufferRecorder &&) = delete;
CommandBufferRecorder(const CommandBufferRecorder &) = delete;
```

### Mauvais exemple

```cpp
// Gestion manuelle - risque de fuite de ressources
vk::CommandBuffer cmd = pool.allocate();
cmd.begin(beginInfo);
cmd.bindPipeline(pipeline);
cmd.draw(3, 1, 0, 0);
// Oubli de cmd.end() possible!
// Ou exception avant end() = état invalide

// Handle nu sans ownership clair
VkPipeline pipeline;
vkCreateGraphicsPipelines(..., &pipeline);
// Qui est responsable de la destruction?
```

### Ownership avec shared_ptr

```cpp
// Les dépendances sont capturées par shared_ptr
class CommandPool : public ObjectWithUniqueHandle<vk::UniqueCommandPool> {
  private:
    // Le Device reste vivant tant que le CommandPool existe
    std::shared_ptr<const Device> m_device;
};

class Buffer {
  private:
    // L'Allocator reste vivant tant que le Buffer existe
    std::shared_ptr<const Allocator> m_allocator;
};
```

---

## 3. Buffers Type-Safe

### Template avec validation compile-time

```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer : public BufferBase {
  public:
    using type = T;
    static constexpr auto host_visible = HostVisible;
    static constexpr auto flags = vk::BufferUsageFlags(Flags);

    // Validation à la compilation
    static consteval bool does_support(vk::BufferUsageFlags usage) {
        return (vk::BufferUsageFlags(flags) & usage) == usage;
    }

    // Méthode disponible UNIQUEMENT si HostVisible == true
    void write(std::span<const T> data, std::size_t offset)
        requires(HostVisible)
    {
        BufferBase::write_bytes(data.data(), data.size_bytes(),
                                offset * sizeof(T));
    }
};
```

### Bon exemple

```cpp
// Types explicites avec alias
using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;
using HostUniformBuffer = Buffer<float, true, UniformBufferUsage>;

// Utilisation type-safe
auto indices = create_buffer<IndexBuffer>(*allocator, 1000);
// indices.write(...);  // ERREUR DE COMPILATION: pas HostVisible

auto uniforms = create_buffer<HostUniformBuffer>(*allocator, 100);
uniforms.write(42.0f, 0);  // OK: float, HostVisible = true
uniforms.write(glm::vec3{}, 0);  // ERREUR: type incorrect
```

### Mauvais exemple

```cpp
// Buffer générique sans type-safety
class UnsafeBuffer {
    void *data;
    size_t size;

    void write(const void *src, size_t bytes) {
        memcpy(data, src, bytes);  // Aucune vérification de type
    }
};

UnsafeBuffer buffer;
buffer.write(&someFloat, sizeof(float));
buffer.write(&someInt, sizeof(int));  // Pas d'erreur, mais potentiellement incorrect

// Paramètres inversés - compilera mais sera faux
void create_buffer(size_t size, VkBufferUsageFlags usage, bool hostVisible);
create_buffer(VK_BUFFER_USAGE_VERTEX_BIT, 1000, true);  // size et usage inversés!
```

---

## 4. Gestion des Erreurs

### Hiérarchie d'exceptions avec source_location

```cpp
class Exception : public std::exception {
  public:
    // source_location capturé automatiquement sans macro
    explicit Exception(
        std::string message,
        std::source_location location = std::source_location::current());

    const std::source_location &location() const noexcept;
};

// Exceptions spécialisées par API
class VulkanException : public Exception {
    vk::Result m_result;
};

class SDLException : public Exception {
    std::string m_sdl_error;  // Capture automatique de SDL_GetError()
};
```

### Bon exemple - Fonctions de vérification

```cpp
// Utilisation des helpers check_*
void create_pipeline() {
    auto result = device.createPipeline(info);
    check_vk(result, "Failed to create pipeline");
    // Si erreur: VulkanException avec fichier, ligne, fonction
}

// Surcharges pour différents types de retour
auto swapchain = check_vk(
    device.createSwapchainKHR(createInfo),
    "Swapchain creation"
);

// SDL avec capture automatique de l'erreur
auto *window = check_sdl(
    SDL_CreateWindow("Title", 800, 600, SDL_WINDOW_VULKAN),
    "Window creation"
);
```

### Mauvais exemple

```cpp
// Vérification manuelle répétitive et verbeuse
vk::Result result = device.createPipeline(info, &pipeline);
if (result != vk::Result::eSuccess) {
    std::cerr << "Error at " << __FILE__ << ":" << __LINE__ << std::endl;
    throw std::runtime_error("Pipeline creation failed");
}

// Pas de contexte, pas de source_location
if (!SDL_Init(SDL_INIT_VIDEO)) {
    throw std::runtime_error(SDL_GetError());
    // On perd l'info de localisation
}

// Ignorer les erreurs silencieusement
device.createBuffer(info, &buffer);  // Pas de vérification!
```

---

## 5. Types Forts (Strong Types)

### Utilisation d'enums vides pour la type-safety

```cpp
// Définition dans 3rd_party.h
enum class Width {};
enum class Height {};
enum class Depth {};
enum class MipLevel {};
```

### Bon exemple

```cpp
// Impossible d'inverser width et height
auto image = allocator->create_image_2D(
    Width{256},
    Height{512},
    false,
    vk::Format::eR8G8B8A8Unorm,
    vk::ImageUsageFlagBits::eSampled
);

// Les types sont explicites dans la signature
Image(vk::Image image, Width width, Height height, Depth depth,
      MipLevel mip_level, vk::Format format, ...);
```

### Mauvais exemple

```cpp
// Paramètres facilement interchangeables
auto image = allocator->create_image_2D(
    256,   // width ou height?
    512,   // height ou width?
    false,
    format,
    usage
);

// Erreur silencieuse - compilation OK mais résultat incorrect
Image create_image(int width, int height, int depth, int mipLevels);
auto img = create_image(512, 256, 1, 4);  // width/height inversés?
```

---

## 6. Conventions de Nommage

### Règles

| Élément | Convention | Exemple |
|---------|------------|---------|
| Classes/Structs | PascalCase | `CommandPool`, `GraphicsPipelineBuilder` |
| Fonctions | snake_case | `create_buffer()`, `write_bytes()` |
| Variables | snake_case | `vertex_count`, `buffer_size` |
| Membres privés | m_ prefix | `m_device`, `m_handle` |
| Constantes | snake_case | `static constexpr auto host_visible` |
| Namespaces | lowercase | `vw`, `vw::rt`, `vw::Model` |

### Bon exemple

```cpp
namespace vw {

class CommandPool {
  public:
    std::vector<vk::CommandBuffer> allocate(std::size_t count);
    void reset_pool();

  private:
    std::shared_ptr<const Device> m_device;
    vk::UniqueCommandPool m_handle;
};

} // namespace vw
```

### Mauvais exemple

```cpp
namespace VW {  // Namespace en majuscules

class commandPool {  // Classe en minuscules
  public:
    std::vector<vk::CommandBuffer> Allocate(std::size_t Count);  // Majuscules incohérentes
    void ResetPool();

  private:
    std::shared_ptr<const Device> device;   // Pas de préfixe m_
    vk::UniqueCommandPool _handle;          // Préfixe _ au lieu de m_
};

}
```

---

## 7. Fonctionnalités C++23

### Concepts pour les contraintes

```cpp
// Définition d'un concept
template <typename T>
concept Vertex =
    std::is_standard_layout_v<T> &&
    std::is_trivially_copyable_v<T> &&
    requires(T x) {
        { T::binding_description(0) } -> std::same_as<vk::VertexInputBindingDescription>;
        { T::attribute_descriptions(0, 0)[0] } -> std::convertible_to<vk::VertexInputAttributeDescription>;
    };

// Utilisation du concept
template <Vertex V>
GraphicsPipelineBuilder &add_vertex_binding() {
    // Erreur de compilation si V ne satisfait pas Vertex
}
```

### requires clauses pour les méthodes conditionnelles

```cpp
// Méthode disponible uniquement si HostVisible == true
void write(std::span<const T> data, std::size_t offset)
    requires(HostVisible)
{
    // ...
}

// Alternative avec if constexpr pour le comportement conditionnel
auto handle() const noexcept {
    if constexpr (sizeof(T) > sizeof(void *))
        return *m_handle;
    else
        return m_handle;
}
```

### Ranges et Views

```cpp
// Bon: utilisation des ranges
auto physicalDevices = devices
    | std::views::filter(supportVersion)
    | to<std::vector>;

auto handles = buffers | std::views::transform(&Buffer::handle);

auto it = std::ranges::find_if(m_buffer_list, has_enough_place);

// Mauvais: boucles manuelles équivalentes
std::vector<PhysicalDevice> physicalDevices;
for (const auto &device : devices) {
    if (supportVersion(device)) {
        physicalDevices.push_back(device);
    }
}
```

### constexpr et consteval

```cpp
// constexpr: peut être évalué à la compilation
static constexpr auto flags = vk::BufferUsageFlags(Flags);

// consteval: DOIT être évalué à la compilation
static consteval bool does_support(vk::BufferUsageFlags usage) {
    return (vk::BufferUsageFlags(flags) & usage) == usage;
}

// Utilisation
static_assert(IndexBuffer::does_support(vk::BufferUsageFlagBits::eIndexBuffer));
```

### Fold expressions

```cpp
// Calcul de taille avec fold expression
template <typename... Ts>
static constexpr auto binding_description(int binding) {
    return vk::VertexInputBindingDescription(
        binding,
        (sizeof(Ts) + ...),  // Fold: sizeof(T1) + sizeof(T2) + ...
        vk::VertexInputRate::eVertex
    );
}
```

---

## Résumé des Anti-Patterns à Éviter

| Anti-Pattern | Problème | Solution |
|--------------|----------|----------|
| Constructeurs avec >3 paramètres | Ordre confus, erreurs silencieuses | Builder pattern |
| Handles Vulkan nus | Fuites mémoire, ownership flou | ObjectWithUniqueHandle |
| `void*` et casts manuels | Pas de type-safety | Templates typés |
| Exceptions sans contexte | Debug difficile | source_location |
| Paramètres int pour dimensions | Inversion possible | Strong types (Width, Height) |
| Boucles manuelles | Verbeux, error-prone | Ranges et views |
| Code dupliqué par type | Maintenance difficile | Concepts et templates |

---

## Fichiers de Référence

- **Builders**: `VulkanWrapper/include/VulkanWrapper/Vulkan/Instance.h`, `DeviceFinder.h`
- **RAII**: `VulkanWrapper/include/VulkanWrapper/Utils/ObjectWithHandle.h`
- **Buffers**: `VulkanWrapper/include/VulkanWrapper/Memory/Buffer.h`
- **Erreurs**: `VulkanWrapper/include/VulkanWrapper/Utils/Error.h`
- **Vertex concept**: `VulkanWrapper/include/VulkanWrapper/Descriptors/Vertex.h`
