# Améliorations potentielles — VulkanWrapper

Ce document recense les axes d'amélioration identifiés dans le projet, classés par catégorie et priorité.

---

## 1. Tests

### Couverture manquante

Les modules suivants n'ont aucun test unitaire :
- **Command** (`CommandBuffer`, `CommandPool`)
- **Model** (`Mesh`, `MeshManager`, `Importer`)
- **Window** (`Window`, `SDL_Initializer`)
- **Vulkan** (`Device`, `Queue`, `PresentQueue`, `Swapchain`)
- **Descriptors** (`DescriptorPool`, `DescriptorAllocator`)

### Tests d'intégration

Il n'existe aucun test de pipeline complet (rendu deferred + ray tracing bout en bout). Des tests d'intégration permettraient de valider les interactions entre modules (ex. : `StagingBufferManager` -> `Transfer` -> `Image` -> `RenderPass`).

### Tests négatifs et cas d'erreur

La plupart des tests valident le chemin nominal. Ajouter des tests pour :
- Création de buffers avec des tailles invalides (0, overflow)
- Compilation de shaders invalides (erreurs de syntaxe GLSL, includes manquants)
- Récupération après perte de device (`VK_ERROR_DEVICE_LOST`)
- Dépassement de la limite de textures bindless (`MAX_TEXTURES`)

### Benchmarks de performance

Aucun benchmark n'existe actuellement. Il serait utile de mesurer :
- Temps d'allocation de buffers (staging vs device-local)
- Débit de transfert CPU -> GPU via `StagingBufferManager`
- Temps de construction BLAS/TLAS pour différentes tailles de scènes
- Overhead du `ResourceTracker` par rapport à des barrières manuelles

---

## 2. Architecture et conception

### Simplifier la hiérarchie du système de matériaux

Le chemin `Model/Material/Internal/` crée un niveau d'imbrication profond. Envisager d'aplatir `Internal/` en rendant `MaterialInfo` et `MeshInfo` privés via le pattern Pimpl plutôt que par un sous-dossier séparé.

### Réduire la complexité de `GraphicsPipelineBuilder`

`GraphicsPipelineBuilder` (`Pipeline/Pipeline.h`) accumule 14 membres avec un état d'initialisation dispersé. Regrouper les paramètres en sous-structures de configuration :

```cpp
struct VertexInputConfig { /* ... */ };
struct RasterizationConfig { /* ... */ };
struct ColorBlendConfig { /* ... */ };  // existe déjà

class GraphicsPipelineBuilder {
    VertexInputConfig m_vertex_input;
    RasterizationConfig m_rasterization;
    // ...
};
```

### Injection de dépendances

Les exemples créent leurs dépendances de manière ad-hoc. Un système léger de composition root (ou simplement une struct regroupant les ressources partagées) améliorerait la testabilité et réduirait le couplage dans les exemples avancés.

### Fonctions utilitaires pour le pattern Visitor

Le `ResourceTracker` utilise `std::variant<ImageState, BufferState, AccelerationStructureState>` sans fournir de helpers pour le matching. Ajouter des fonctions utilitaires ou des surcharges typées réduirait le code boilerplate côté utilisateur.

---

## 3. Qualité du code

### Duplication dans les hash functors

`ResourceTracker.h` définit trois structs de hash distinctes (`HandleHash`, `BufferHandleHash`, `ImageHandleHash`) avec une logique similaire. Les unifier via un template générique :

```cpp
template <typename T>
struct VkHandleHash {
    size_t operator()(T handle) const noexcept {
        return std::hash<uint64_t>{}(reinterpret_cast<uint64_t>(
            static_cast<typename T::CType>(handle)));
    }
};
```

### Paramètres optionnels dans `Transfer`

`Transfer::blit()` accepte 3 paramètres optionnels. Remplacer par un builder ou une struct de configuration pour améliorer la lisibilité :

```cpp
transfer.blit(cmd, src, dst, BlitOptions{.filter = eLinear, .src_mip = 0});
```

### Cohérence de la version CMake minimum

Le `CMakeLists.txt` racine exige CMake 3.25, mais `tests/CMakeLists.txt` spécifie 3.20. Harmoniser sur une seule version.

### Commentaires en français dans CMake

La fonction `VwCompileShader()` contient des commentaires en français. Pour un projet public, uniformiser la langue des commentaires (anglais recommandé).

---

## 4. Utilisation moderne du C++23

### `std::expected<T, E>` pour les résultats faillibles

Le projet utilise exclusivement des exceptions. Pour les opérations à haute fréquence ou les chemins critiques en performance, `std::expected` offrirait une alternative sans overhead :

```cpp
// Actuel
auto module = compiler.compile_file_to_module(device, "shader.frag");
// Possible
auto module = compiler.try_compile_file_to_module(device, "shader.frag");
// -> std::expected<ShaderModule, ShaderCompilationError>
```

Cela n'exclut pas de conserver les exceptions pour les erreurs irrécupérables (perte de device, out of memory).

### `std::ranges` plus systématique

Le codebase utilise `std::ranges` et `std::views` par endroits mais revient à des boucles classiques dans d'autres. Identifier et convertir les boucles `for` qui itèrent + transforment + filtrent vers des pipelines de ranges.

### Remplacement des macros par `constexpr` / `consteval`

Le système d'enregistrement des types de matériaux (`MaterialTypeTag.h`) utilise des macros. C++23 permet de remplacer cela par des fonctions `consteval` ou des variables `constexpr` avec identification automatique de type.

### Agrégats avec initialisation désignée

Certaines structures complexes (options de blit, configuration de pipeline) bénéficieraient de designated initializers pour améliorer la lisibilité à l'appel :

```cpp
auto options = BlitOptions{.src_mip = MipLevel{0}, .filter = vk::Filter::eLinear};
```

---

## 5. Shaders

### Include guards manquants

`random.glsl` utilise un include guard (`#ifndef RANDOM_GLSL`), mais les autres fichiers inclus (`atmosphere_common.glsl`, `atmosphere_functions.glsl`, `geometry_access.glsl`, `sky_radiance.glsl`) n'en ont pas. Ajouter des guards à tous les fichiers d'inclusion GLSL.

### Documentation des shaders d'atmosphère

Les fichiers `atmosphere_common.glsl` (~119 lignes) et `atmosphere_functions.glsl` (~174 lignes) implémentent des calculs physiques complexes (diffusion de Rayleigh, Mie) sans documentation inline. Ajouter :
- Références aux papiers académiques utilisés
- Description des paramètres physiques et de leurs unités
- Schémas ASCII des coordonnées et conventions

### Validation des shaders en CI

Aucune étape de validation/linting des shaders n'est configurée dans le processus de build. Ajouter :
- Compilation de tous les shaders pendant le build (pas seulement ceux référencés par les targets)
- Vérification des include guards
- Optionnel : analyse statique avec `glslangValidator --aml`

---

## 6. Performance

### Cache d'`ImageViewBuilder` dans `Subpass`

`Subpass::get_or_create_image()` reconstruit un `ImageViewBuilder` à chaque appel de cache miss. Stocker la configuration du builder pour éviter la reconstruction.

### Conversion de format dans `Transfer::saveToFile()`

`saveToFile()` alloue potentiellement un buffer intermédiaire RGBA pour la conversion de format. Pour les cas fréquents (screenshots, debug), envisager :
- Un pool de buffers de conversion réutilisables
- Un chemin rapide quand le format source est déjà compatible

### Stratégie d'éviction du cache d'images

`Subpass` stocke les images en cache via `shared_ptr` sans stratégie d'éviction explicite au-delà du changement de dimensions. Si la fenêtre est redimensionnée fréquemment, les anciennes images restent en mémoire jusqu'à la destruction du `Subpass`. Ajouter une éviction basée sur un compteur de frames ou un LRU.

### Écriture de descripteurs dans le système de matériaux

Les descriptor writes du système bindless pourraient s'accumuler si `update_descriptors()` n'est pas appelé régulièrement. Documenter le contrat d'utilisation ou forcer un flush périodique.

---

## 7. API publique

### Simplifier `create_buffer<>` avec des concepts

`AllocateBufferUtils.h` expose trois surcharges de `create_buffer<>`. Les unifier avec un concept :

```cpp
template <typename T, BufferConfig Config>
auto create_buffer(Allocator& allocator, size_t count);
```

où `BufferConfig` encode visibilité et usage via un concept.

### Rendre les extensions optionnelles plus déclaratives

`DeviceFinder` requiert des appels de méthode explicites pour chaque extension. Un système déclaratif serait plus ergonomique :

```cpp
auto device = instance->findGpu()
    .with_queue(eGraphics)
    .require(Feature::Synchronization2, Feature::DynamicRendering)
    .optional(Feature::RayTracing)
    .build();
```

### Typage plus fort pour `Allocator::allocate_buffer()`

`allocate_buffer()` prend un `bool host_visible` en paramètre. Remplacer par un enum :

```cpp
enum class MemoryLocation { DeviceLocal, HostVisible };
```

---

## 8. Documentation

### Documentation inline des headers publics

La majorité des méthodes publiques dans les modules Pipeline, Descriptor et Image manquent de commentaires Doxygen. Prioriser :
- `GraphicsPipelineBuilder` (API complexe avec beaucoup d'options)
- `ResourceTracker` (concepts de synchronisation non triviaux)
- `BindlessMaterialManager` / `BindlessTextureManager` (architecture bindless)

### Guide de migration Vulkan 1.0 -> 1.3

Le fichier `CLAUDE.md` liste les anti-patterns (synchronization v1, render pass legacy) mais ne fournit pas de guide de migration pour les contributeurs habitués à l'ancienne API.

### Guide des bonnes pratiques ray tracing

Le module ray tracing est fonctionnel mais manque de documentation sur :
- Quand reconstruire vs mettre à jour un BLAS/TLAS
- Stratégies de compaction d'acceleration structures
- Bonnes pratiques pour le shader binding table

### Guide de performance

Documenter les patterns recommandés pour :
- Taille optimale des staging buffers
- Batch de transferts vs transferts individuels
- Gestion du budget mémoire GPU

---

## 9. Build et CI

### Épingler les versions vcpkg

Aucun fichier `vcpkg.json` avec des contraintes de version n'est présent. Les dépendances transitives peuvent varier entre les builds. Ajouter un `builtin-baseline` et des `version>=` dans `vcpkg.json`.

### Vérification de la disponibilité de shaderc

Le CMake utilise `Vulkan_GLSLANG_VALIDATOR_EXECUTABLE` sans vérifier sa présence. Ajouter un `find_program()` avec un message d'erreur explicite.

### CI/CD

Mettre en place une pipeline CI avec :
- Build sur Linux (llvmpipe) et Windows
- Exécution des tests unitaires
- Compilation de tous les shaders
- Vérification du formatage (`clang-format --dry-run --Werror`)
- Analyse statique (`clang-tidy`)

---

## 10. Sécurité et robustesse

### Détection de fuites de ressources

Aucun mécanisme de détection de fuites n'existe dans les destructeurs. En mode debug, ajouter des assertions vérifiant que les handles Vulkan ont été correctement libérés avant la destruction de l'`Allocator` ou du `Device`.

### Validation des handles après move

Après un `std::move`, les handles ne sont pas systématiquement remis à `VK_NULL_HANDLE`. Vérifier que tous les types déplaçables (Image, Buffer) nullifient leurs handles source.

### Limites du système bindless

`MAX_TEXTURES` est fixé à 4096 sans mécanisme de détection de dépassement à l'exécution. Ajouter une assertion ou une exception claire si la limite est atteinte.

---

## Résumé par priorité

| Priorité | Amélioration | Impact |
|----------|-------------|--------|
| Haute | Ajouter des tests pour les modules non couverts | Fiabilité |
| Haute | Include guards dans tous les shaders GLSL | Correction |
| Haute | Épingler les versions vcpkg | Reproductibilité |
| Moyenne | Simplifier `GraphicsPipelineBuilder` | Maintenabilité |
| Moyenne | Documentation inline des headers publics | Accessibilité |
| Moyenne | Tests d'intégration pipeline complet | Fiabilité |
| Moyenne | `std::expected` pour les chemins critiques | Performance |
| Moyenne | CI/CD avec build + tests + lint | Qualité |
| Basse | Aplatir la hiérarchie Material/Internal | Simplicité |
| Basse | Remplacer les macros par `consteval` | Modernité |
| Basse | Benchmarks de performance | Optimisation |
| Basse | Guide de migration Vulkan 1.0 -> 1.3 | Documentation |
