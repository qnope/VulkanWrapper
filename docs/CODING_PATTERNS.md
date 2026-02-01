# Bonnes Pratiques de Programmation - VulkanWrapper

Ce document résume les patterns et conventions à suivre dans ce projet, avec des exemples et contre-exemples.

## Table des matières

1. [Pattern Builder](#1-pattern-builder)
2. [RAII et Gestion des Ressources](#2-raii-et-gestion-des-ressources)
3. [Typage Fort](#3-typage-fort)
4. [Gestion des Erreurs](#4-gestion-des-erreurs)
5. [Conventions de Nommage](#5-conventions-de-nommage)
6. [Buffers Type-Safe](#6-buffers-type-safe)

---

## 1. Pattern Builder

### Principe

Tous les objets complexes utilisent des builders fluides avec chaînage de méthodes.

### Bonne pratique

```cpp
// Chaque méthode retourne une référence au builder
class InstanceBuilder {
public:
    InstanceBuilder &addPortability();
    InstanceBuilder &setDebug();
    InstanceBuilder &setApiVersion(ApiVersion version);

    std::shared_ptr<Instance> build();  // Finalise la construction
};

// Utilisation fluide
auto instance = vw::InstanceBuilder()
    .addPortability()
    .setApiVersion(vw::ApiVersion::e13)
    .setDebug()
    .build();
```

### Contre-exemple

```cpp
// MAL : Constructeur avec trop de paramètres
Instance(bool portability, bool debug, ApiVersion version,
         std::vector<const char*> extensions);

// MAL : Setters séparés qui ne retournent rien
InstanceBuilder builder;
builder.setPortability(true);  // void
builder.setDebug(true);        // void - pas de chaînage possible
auto instance = builder.build();

// MAL : build() retourne un raw pointer
Instance* build();  // Qui gère la mémoire ?
```

### Règles

- Chaque méthode du builder retourne `*this` ou `Builder&`
- `build()` retourne un `std::shared_ptr` ou un objet par valeur
- Le constructeur de l'objet final est privé, le builder est `friend`

---

## 2. RAII et Gestion des Ressources

### Principe

Les ressources Vulkan sont encapsulées avec destruction automatique.

### Bonne pratique

```cpp
// Classe de base pour les handles Vulkan
template <typename UniqueHandle>
class ObjectWithUniqueHandle {
public:
    auto handle() const noexcept { return *m_handle; }
private:
    UniqueHandle m_handle;  // vk::UniqueFence, vk::UniqueImage, etc.
};

// Exemple : Fence avec cleanup automatique
class Fence : public ObjectWithUniqueHandle<vk::UniqueFence> {
public:
    Fence(const Fence &) = delete;            // Non copiable
    Fence(Fence &&) noexcept = default;       // Déplaçable
    Fence &operator=(const Fence &) = delete;
    ~Fence();  // Cleanup via vk::UniqueFence

private:
    vk::Device m_device;
};
```

### Contre-exemple

```cpp
// MAL : Raw handle sans gestion de lifetime
class Fence {
public:
    vk::Fence handle;  // Qui détruit ? Quand ?
};

// MAL : Copiable par défaut - double destruction
class Image {
    VmaAllocation m_allocation;
    // Pas de delete du copy constructor = double free
};

// MAL : Destruction manuelle
void cleanup() {
    vkDestroyFence(device, fence, nullptr);  // Peut être oublié
}
```

### Règles

- Utiliser `vk::Unique*` pour les handles Vulkan
- Supprimer explicitement copy constructor et copy assignment
- Permettre le move semantics avec `= default`
- Utiliser `std::shared_ptr` pour le partage de ressources

---

## 3. Typage Fort

### Principe

Utiliser des types distincts pour éviter les confusions de paramètres.

### Bonne pratique

```cpp
// Types forts pour les dimensions
enum class Width {};
enum class Height {};
enum class Depth {};
enum class MipLevel {};

// Impossible de confondre width et height
class Image {
public:
    Image(Width w, Height h, Depth d);
private:
    Width m_width;
    Height m_height;
};

// Utilisation claire
auto image = createImage(Width(1024), Height(768), Depth(1));
```

### Contre-exemple

```cpp
// MAL : Types primitifs - facile de se tromper
class Image {
public:
    Image(int width, int height, int depth);
};

// Erreur silencieuse : height et width inversés
auto image = createImage(768, 1024, 1);

// MAL : Aliases qui ne protègent pas
using Width = int;
using Height = int;
// Width et Height sont interchangeables !
```

### Règles

- Utiliser `enum class` vides pour les types dimensionnels
- Préférer les types forts aux aliases simples
- Nommer les paramètres clairement dans l'API publique

---

## 4. Gestion des Erreurs

### Principe

Hiérarchie d'exceptions spécifiques avec capture automatique de la localisation.

### Bonne pratique

```cpp
// Hiérarchie d'exceptions
class Exception : public std::exception {
public:
    explicit Exception(
        std::string message,
        std::source_location location = std::source_location::current());

    const std::source_location &location() const noexcept;
};

class VulkanException : public Exception { /*...*/ };
class SDLException : public Exception { /*...*/ };

// Fonctions helper (pas de macros!)
inline void check_vk(vk::Result result, std::string_view context,
                     std::source_location loc = std::source_location::current()) {
    if (result != vk::Result::eSuccess) {
        throw VulkanException(result, std::string(context), loc);
    }
}

// Utilisation
auto buffer = device.createBuffer(info);
check_vk(buffer.result, "Failed to create buffer");
```

### Contre-exemple

```cpp
// MAL : Codes d'erreur ignorables
int createBuffer(...) {
    if (error) return -1;  // Facilement ignoré
    return 0;
}

// MAL : Macros pour les erreurs
#define CHECK_VK(result) \
    if (result != VK_SUCCESS) throw std::runtime_error("Error")
// Difficile à débugger, pas de contexte

// MAL : Exception générique
throw std::runtime_error("Something failed");
// Pas de type spécifique, pas de localisation

// MAL : Ignorer les erreurs
vkCreateBuffer(...);  // Résultat ignoré
```

### Règles

- Une classe d'exception par domaine (Vulkan, SDL, VMA, etc.)
- Utiliser `std::source_location` pour la traçabilité
- Fonctions helper `check_*` au lieu de macros
- Toujours vérifier les résultats Vulkan

---

## 5. Conventions de Nommage

### Règles

| Élément | Convention | Exemple |
|---------|------------|---------|
| Classes | PascalCase | `GraphicsPipelineBuilder` |
| Fonctions | snake_case | `create_buffer()` |
| Variables | snake_case | `buffer_size` |
| Membres | m_snake_case | `m_device` |
| Concepts | PascalCase | `Vertex` |
| Namespaces | minuscules | `vw::Model::Material` |
| Constantes | PascalCase ou UPPER_CASE | `MaxFrames` |

### Bonne pratique

```cpp
namespace vw {

class CommandPoolBuilder {  // PascalCase
public:
    CommandPoolBuilder &with_queue(Queue &queue);  // snake_case
    CommandPool build();

private:
    std::shared_ptr<Device> m_device;  // m_ prefix
};

template <typename T>
concept Vertex = requires(T v) {  // PascalCase pour concepts
    { v.position } -> std::convertible_to<glm::vec3>;
};

}  // namespace vw
```

### Contre-exemple

```cpp
// MAL : Mélange de conventions
class command_pool_Builder {  // Incohérent
    std::shared_ptr<Device> device;  // Pas de m_ prefix

    commandPool Build();  // camelCase au lieu de snake_case
};

// MAL : Préfixes hongrois
int iBufferCount;
float fWidth;
bool bIsValid;
```

---

## 6. Buffers Type-Safe

### Principe

Les buffers sont paramétrés par type d'élément, visibilité CPU et usage.

### Bonne pratique

```cpp
// Buffer avec 3 paramètres template
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer : public BufferBase {
public:
    // Méthodes conditionnelles avec requires
    void write(std::span<const T> data, std::size_t offset)
        requires(HostVisible);  // Compile uniquement si host visible

    std::size_t size() const noexcept {
        return size_bytes() / sizeof(T);  // Taille en éléments
    }

    // Vérification compile-time
    static consteval bool does_support(vk::BufferUsageFlags usage) {
        return (vk::BufferUsageFlags(Flags) & usage) == usage;
    }
};

// Aliases pour cas communs
using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;
using VertexBuffer = Buffer<Vertex, false, VertexBufferUsage>;

// Utilisation
auto indices = create_buffer<IndexBuffer>(allocator, 1000);
// indices.write(...);  // Erreur : pas host visible!
```

### Contre-exemple

```cpp
// MAL : Buffer générique sans type safety
class Buffer {
public:
    void write(void *data, size_t bytes);  // Pas de type checking
    size_t size();  // En bytes ou en éléments ?
};

// MAL : Flags runtime
Buffer createBuffer(int elementSize, bool hostVisible, int usageFlags);
// Erreurs détectées seulement à l'exécution

// MAL : Cast dangereux
auto buffer = createBuffer<float>(...);
buffer.write(reinterpret_cast<float*>(intData), count);  // UB
```

### Règles

- Paramétrer par `<Type, HostVisible, Usage>`
- Utiliser `requires` pour les méthodes conditionnelles
- `consteval` pour les vérifications compile-time
- Taille toujours exprimée en éléments, pas en bytes

---

## Résumé des Anti-Patterns à Éviter

| Anti-Pattern | Alternative |
|--------------|-------------|
| Raw pointers | `std::shared_ptr`, `std::unique_ptr` |
| Macros pour erreurs | Fonctions avec `std::source_location` |
| Types primitifs pour dimensions | `enum class Width/Height` |
| Constructeurs à nombreux paramètres | Pattern Builder |
| Copie de ressources GPU | Delete copy, default move |
| `void*` et casts | Templates type-safe |
| Destruction manuelle | RAII avec `vk::Unique*` |
| Flags runtime | Template parameters |

---

## Fichiers de Référence

Pour voir ces patterns en action :

- **Builder** : `VulkanWrapper/include/VulkanWrapper/Vulkan/Instance.h`
- **RAII** : `VulkanWrapper/include/VulkanWrapper/Utils/ObjectWithHandle.h`
- **Erreurs** : `VulkanWrapper/include/VulkanWrapper/Utils/Error.h`
- **Buffers** : `VulkanWrapper/include/VulkanWrapper/Memory/Buffer.h`
- **Types forts** : `VulkanWrapper/include/VulkanWrapper/3rd_party.h`
