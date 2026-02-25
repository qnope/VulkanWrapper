# Guide de migration vers les modules C++20

## Table des matières

1. [Introduction aux modules C++20](#1-introduction-aux-modules-c20)
2. [Concepts fondamentaux](#2-concepts-fondamentaux)
3. [Prérequis toolchain](#3-prérequis-toolchain)
4. [Support CMake](#4-support-cmake)
5. [Anatomie d'un module](#5-anatomie-dun-module)
6. [Stratégie de migration pour VulkanWrapper](#6-stratégie-de-migration-pour-vulkanwrapper)
7. [Phase 1 — Configuration CMake](#7-phase-1--configuration-cmake)
8. [Phase 2 — Module wrapper pour les dépendances tierces](#8-phase-2--module-wrapper-pour-les-dépendances-tierces)
9. [Phase 3 — Migration module par module](#9-phase-3--migration-module-par-module)
10. [Phase 4 — Module principal et partitions](#10-phase-4--module-principal-et-partitions)
11. [Phase 5 — Migration des tests et exemples](#11-phase-5--migration-des-tests-et-exemples)
12. [Gestion des templates et du code header-only](#12-gestion-des-templates-et-du-code-header-only)
13. [Pièges courants et solutions](#13-pièges-courants-et-solutions)
14. [Coexistence headers/modules pendant la transition](#14-coexistence-headersmodules-pendant-la-transition)
15. [Impact sur le PCH existant](#15-impact-sur-le-pch-existant)
16. [Checklist de migration complète](#16-checklist-de-migration-complète)

---

## 1. Introduction aux modules C++20

### Pourquoi les modules ?

Le modèle classique C++ repose sur `#include` et le préprocesseur : chaque
`.cpp` inclut des headers, qui sont copiés textuellement puis recompilés à
chaque fois. Cela pose plusieurs problèmes :

- **Temps de compilation** : `vulkan.hpp` fait ~80 000 lignes. Chaque `.cpp`
  qui l'inclut le recompile (le PCH atténue, mais ne résout pas le problème
  fondamentalement).
- **Fragilité** : l'ordre des `#include` peut changer le comportement (macros,
  redéfinitions).
- **Fuite de détails** : un `#include` expose tout le contenu du header, même
  ce qui est interne.
- **Macros globales** : les `#define` d'un header polluent tous ceux qui
  suivent.

Les **modules C++20** résolvent ces problèmes :

```
┌──────────────────────────────────────────────────────────────────┐
│                    Modèle classique (#include)                   │
│                                                                  │
│  A.cpp ──include──▶ vulkan.hpp (80k lignes) ──parse──▶ compile  │
│  B.cpp ──include──▶ vulkan.hpp (80k lignes) ──parse──▶ compile  │
│  C.cpp ──include──▶ vulkan.hpp (80k lignes) ──parse──▶ compile  │
│                     ↑ parsé 3 fois                               │
├──────────────────────────────────────────────────────────────────┤
│                    Modèle modules (import)                       │
│                                                                  │
│  vulkan.cppm ──compile──▶ BMI (binaire, une seule fois)         │
│                                                                  │
│  A.cpp ──import──▶ BMI ──▶ compile (instantané)                 │
│  B.cpp ──import──▶ BMI ──▶ compile (instantané)                 │
│  C.cpp ──import──▶ BMI ──▶ compile (instantané)                 │
└──────────────────────────────────────────────────────────────────┘
```

Un module est compilé **une seule fois** en un fichier binaire intermédiaire
(BMI — *Binary Module Interface*). Les consommateurs importent ce BMI quasi
instantanément.

### Avantages concrets pour VulkanWrapper

| Aspect | Avant (headers) | Après (modules) |
|--------|-----------------|-----------------|
| Temps de build incrémental | Recompile quand un header change + tous ses dépendants | Seul le module modifié recompile, les dépendants ne recompilent que si l'interface change |
| Isolation | `3rd_party.h` expose GLM, Vulkan, STL partout | Chaque module contrôle exactement ce qu'il exporte |
| Macros | `VULKAN_HPP_*`, `GLM_FORCE_*` doivent être définies avant l'include | Les macros ne traversent pas les frontières de modules |
| PCH | Nécessaire pour garder des temps raisonnables | Rendu inutile par le BMI |

---

## 2. Concepts fondamentaux

### 2.1. Module Interface Unit (`.cppm`)

C'est le **fichier public** d'un module. Il déclare ce que le module exporte.
Extension conventionnelle : `.cppm` (Clang/MSVC), `.ixx` (MSVC), ou `.mpp`.

```cpp
// fichier: vw_utils.cppm
export module vw.utils;    // ← déclaration du module

// Ce qui est exporté est visible par les consommateurs
export namespace vw {
    int add(int a, int b);
}

// Ce qui n'est PAS exporté est interne au module
namespace vw::detail {
    int internal_helper();  // invisible de l'extérieur
}
```

### 2.2. Module Implementation Unit (`.cpp`)

C'est le fichier d'implémentation, il appartient au module mais n'exporte rien :

```cpp
// fichier: vw_utils.cpp
module vw.utils;    // ← pas de "export", c'est une implémentation

// Accès à tout ce qui est dans le module (exporté ou non)
namespace vw {
    int add(int a, int b) { return a + b; }
}

namespace vw::detail {
    int internal_helper() { return 42; }
}
```

### 2.3. Module Partitions

Un gros module peut être découpé en **partitions** — des sous-unités internes
qui ne sont pas visibles directement de l'extérieur :

```cpp
// ── vw-image.cppm (partition interface) ──
export module vw:image;           // partition "image" du module "vw"

export namespace vw {
    class Image { /* ... */ };
}

// ── vw-memory.cppm (partition interface) ──
export module vw:memory;          // partition "memory" du module "vw"
import :image;                    // import d'une partition sœur

export namespace vw {
    class Allocator { /* ... */ };
}

// ── vw.cppm (module interface principal) ──
export module vw;
export import :image;             // ré-exporte la partition
export import :memory;
```

Les partitions sont **le mécanisme clé** pour structurer un gros module comme
VulkanWrapper.

### 2.4. Global Module Fragment

Pour inclure des headers classiques (bibliothèques tierces qui ne supportent
pas les modules), on utilise le **global module fragment** :

```cpp
module;                           // ← début du global module fragment

// Ici on peut #include des headers classiques
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

export module vw:vulkan_deps;     // ← fin du fragment, début du module
```

C'est ainsi que nous gérerons Vulkan, GLM, SDL, et les autres dépendances qui
n'ont pas encore migré vers les modules.

### 2.5. `import std;`

C++23 introduit `import std;` — un module standard pour toute la STL :

```cpp
export module vw.utils;
import std;                       // remplace tous les #include <vector>, etc.

export namespace vw {
    std::vector<int> make_range(int n);
}
```

Supporté par CMake 3.30+ avec Clang 18+ ou MSVC 17.10+.

### 2.6. Résumé visuel

```
┌─────────────────────────────────────────────────────────────────┐
│ Vocabulaire des modules C++20                                   │
│                                                                 │
│  export module X;        → Module Interface Unit (.cppm)        │
│  module X;               → Module Implementation Unit (.cpp)    │
│  export module X:part;   → Partition Interface Unit (.cppm)     │
│  module X:part;          → Partition Implementation Unit (.cpp) │
│                                                                 │
│  export import X:part;   → Ré-exporte une partition             │
│  import X;               → Importe un module                    │
│  import :part;           → Importe une partition (intra-module) │
│                                                                 │
│  module;                 → Global Module Fragment (pour         │
│  #include <legacy.h>     →   les headers non-modules)           │
│  export module X;        → Fin du fragment                      │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. Prérequis toolchain

### Compilateurs

| Compilateur | Version minimale | Partitions | `import std;` | Notes |
|-------------|-----------------|------------|---------------|-------|
| **Clang** | 16+ (partiel), **18+** (recommandé) | Oui | Oui (18+) | Meilleur support actuel avec CMake |
| **GCC** | 14+ | Oui | Oui (15+) | Support fonctionnel mais moins mature |
| **MSVC** | 17.4+ (VS 2022) | Oui | Oui (17.10+) | Premier compilateur à supporter les modules |

**Recommandation pour VulkanWrapper** : Clang 18+ (cohérent avec le toolchain
actuel du projet qui utilise déjà Clang).

### CMake

| Version | Support |
|---------|---------|
| 3.25 (actuelle du projet) | Aucun support modules |
| 3.28 | Support expérimental (`CMAKE_EXPERIMENTAL_CXX_IMPORT_STD`) |
| **3.30+** | Support natif des modules C++20, `import std;` |
| **3.31+** | Améliorations de performance et stabilité (recommandé) |

**Action requise** : passer `cmake_minimum_required` de `3.25` à `3.30`.

### Ninja

Le **générateur Ninja** (1.11+) est **obligatoire** pour les modules C++20 avec
CMake. Le générateur Makefile ne gère pas correctement le graphe de dépendances
des modules (il ne sait pas que `A.cppm` doit être compilé avant `B.cppm` si B
importe A).

```bash
# Vérifier la version de Ninja
ninja --version    # doit être >= 1.11

# Installer si nécessaire
sudo apt install ninja-build    # Linux
brew install ninja               # macOS
```

---

## 4. Support CMake

### 4.1. Configuration de base

Voici les changements CMake nécessaires au niveau du projet :

```cmake
# CMakeLists.txt (racine)
cmake_minimum_required(VERSION 3.30)    # ← était 3.25

project(VulkanWrapper LANGUAGES CXX)

# OBLIGATOIRE : C++23 (pour import std;) ou au minimum C++20
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### 4.2. `target_sources` avec `FILE_SET`

CMake 3.28+ introduit `FILE_SET` de type `CXX_MODULES` pour déclarer les
fichiers de module :

```cmake
add_library(VulkanWrapperCoreLibrary)

target_sources(VulkanWrapperCoreLibrary
    PUBLIC
        FILE_SET modules TYPE CXX_MODULES FILES
            src/vw.cppm                      # module principal
            src/vw-utils.cppm                # partition
            src/vw-image.cppm                # partition
            # ...
    PRIVATE
        src/vw-utils_impl.cpp               # implémentation
        src/vw-image_impl.cpp
)
```

Points clés :

- `FILE_SET ... TYPE CXX_MODULES` : indique à CMake que ces fichiers sont des
  module interface units
- `PUBLIC` : les modules sont visibles par les cibles qui link cette library
- `PRIVATE` : les fichiers `.cpp` d'implémentation restent internes
- CMake analyse automatiquement les `import` / `export module` pour construire
  le graphe de dépendances

### 4.3. `import std;`

Pour activer `import std;` :

```cmake
# CMake 3.30+
target_compile_features(VulkanWrapperCoreLibrary PUBLIC cxx_std_23)

# CMake construit automatiquement le module std si le compilateur le supporte
```

Avec Clang, il faut aussi s'assurer que la libc++ est installée avec le
support modules :

```bash
# Vérifier que le module std existe
ls $(clang++ -print-resource-dir)/../../../share/libc++/v1/std.cppm
```

### 4.4. Générateur Ninja

Configurer le projet avec Ninja :

```bash
cmake -B build -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE=Debug
```

---

## 5. Anatomie d'un module

Avant de plonger dans la migration, regardons la structure complète d'un module
avec un exemple concret tiré du projet.

### Exemple : transformer `Utils/ObjectWithHandle.h` en module

**Avant** (header classique) :

```cpp
// include/VulkanWrapper/Utils/ObjectWithHandle.h
#pragma once
#include "VulkanWrapper/3rd_party.h"

namespace vw {
template <typename T> class ObjectWithHandle {
  public:
    ObjectWithHandle(auto handle) noexcept : m_handle{std::move(handle)} {}
    auto handle() const noexcept {
        if constexpr (sizeof(T) > sizeof(void *))
            return *m_handle;
        else
            return m_handle;
    }
  private:
    T m_handle;
};

template <typename UniqueHandle>
using ObjectWithUniqueHandle = ObjectWithHandle<UniqueHandle>;

constexpr auto to_handle = std::views::transform([](const auto &x) {
    if constexpr (requires { x.handle(); })
        return x.handle();
    else
        return x->handle();
});
} // namespace vw
```

**Après** (module) :

```cpp
// src/VulkanWrapper/Utils/ObjectWithHandle.cppm
module;

// Global module fragment : headers legacy nécessaires
#include <vulkan/vulkan.hpp>    // pour vk::UniqueXxx types
#include <ranges>               // pour std::views::transform

export module vw:utils.object_with_handle;

export namespace vw {

template <typename T> class ObjectWithHandle {
  public:
    ObjectWithHandle(auto handle) noexcept
        : m_handle{std::move(handle)} {}

    auto handle() const noexcept {
        if constexpr (sizeof(T) > sizeof(void *))
            return *m_handle;
        else
            return m_handle;
    }

  private:
    T m_handle;
};

template <typename UniqueHandle>
using ObjectWithUniqueHandle = ObjectWithHandle<UniqueHandle>;

constexpr auto to_handle = std::views::transform([](const auto &x) {
    if constexpr (requires { x.handle(); })
        return x.handle();
    else
        return x->handle();
});

} // namespace vw
```

**Ce qui change** :

| Aspect | Header | Module |
|--------|--------|--------|
| Garde | `#pragma once` | Inutile (importé une seule fois par design) |
| Dépendances | `#include "3rd_party.h"` | Global module fragment + imports ciblés |
| Visibilité | Tout est public | Seul `export` est visible |
| Macros | Polluent le scope global | Confinées au global module fragment |

---

## 6. Stratégie de migration pour VulkanWrapper

### Vue d'ensemble de l'architecture actuelle

```
VulkanWrapper/
├── include/VulkanWrapper/
│   ├── 3rd_party.h              ← PCH + macros Vulkan/GLM
│   ├── fwd.h                    ← forward declarations
│   ├── Utils/                   ← ObjectWithHandle, Error, Algos, Alignment
│   ├── Vulkan/                  ← Instance, Device, Queue, etc.
│   ├── Memory/                  ← Allocator, Buffer, Transfer, etc.
│   ├── Image/                   ← Image, ImageView, Sampler, etc.
│   ├── Command/                 ← CommandPool, CommandBuffer
│   ├── Synchronization/         ← Fence, Semaphore, ResourceTracker
│   ├── Descriptors/             ← DescriptorSet, DescriptorSetLayout, etc.
│   ├── Pipeline/                ← Pipeline, PipelineLayout, ShaderModule, etc.
│   ├── Shader/                  ← ShaderCompiler
│   ├── Model/                   ← Scene, Mesh, Material/...
│   ├── RenderPass/              ← Subpass, ScreenSpacePass, SkyPass, etc.
│   ├── RayTracing/              ← BLAS, TLAS, RayTracingPipeline, etc.
│   ├── Random/                  ← NoiseTexture, RandomSamplingBuffer
│   └── Window/                  ← SDL_Initializer, Window
└── src/VulkanWrapper/           ← (miroir de include/ pour les .cpp)
```

### Stratégie : un module `vw` avec des partitions

On va créer **un seul module** nommé `vw` avec des **partitions** qui
correspondent aux sous-dossiers actuels. C'est l'approche la plus naturelle et
la plus cohérente avec l'architecture existante.

```
Module: vw
├── :third_party          ← global module fragment pour vulkan/glm/sdl
├── :utils                ← Utils/*
├── :vulkan               ← Vulkan/*
├── :memory               ← Memory/*
├── :image                ← Image/*
├── :command              ← Command/*
├── :sync                 ← Synchronization/*
├── :descriptors          ← Descriptors/*
├── :pipeline             ← Pipeline/*
├── :shader               ← Shader/*
├── :model                ← Model/* + Model/Material/* + Model/Internal/*
├── :renderpass           ← RenderPass/*
├── :raytracing           ← RayTracing/*
├── :random               ← Random/*
└── :window               ← Window/*
```

### Graphe de dépendances entre partitions

```
:third_party ──────────────────────────────────────────────────────────┐
     │                                                                 │
     ▼                                                                 │
  :utils ──────────────────────────────────────────────────────────────┤
     │                                                                 │
     ▼                                                                 │
  :vulkan ─────────────────────────────────────────────────────────────┤
     │                                                                 │
     ├──────────────▶ :command                                         │
     │                   │                                             │
     ├──────────────▶ :sync                                            │
     │                   │                                             │
     ├──────────────▶ :memory ◀── :image                               │
     │                   │           │                                 │
     │                   ▼           ▼                                 │
     ├──────────────▶ :descriptors                                     │
     │                   │                                             │
     │                   ▼                                             │
     ├──────────────▶ :pipeline                                        │
     │                   │                                             │
     │                   ├──▶ :shader                                  │
     │                   │                                             │
     │                   ├──▶ :renderpass                               │
     │                   │                                             │
     │                   ├──▶ :model                                   │
     │                   │                                             │
     │                   └──▶ :raytracing                               │
     │                                                                 │
     ├──────────────▶ :random                                          │
     │                                                                 │
     └──────────────▶ :window                                          │
```

### Ordre de migration (feuilles d'abord)

La migration se fait des modules les **plus indépendants** vers les plus
dépendants :

```
Vague 1 (aucune dépendance interne) :
  ├── :utils            (Error, ObjectWithHandle, Algos, Alignment)
  └── :window           (SDL_Initializer, Window)

Vague 2 (dépend de Vague 1) :
  └── :vulkan           (Instance, Device, Queue, PhysicalDevice, ...)

Vague 3 (dépend de Vague 2) :
  ├── :command          (CommandPool, CommandBuffer)
  ├── :sync             (Fence, Semaphore, ResourceTracker)
  └── :memory           (Allocator, Buffer, Transfer, ...)

Vague 4 (dépend de Vague 3) :
  ├── :image            (Image, ImageView, Sampler, ...)
  └── :descriptors      (DescriptorSet, DescriptorSetLayout, ...)

Vague 5 (dépend de Vague 4) :
  ├── :pipeline         (Pipeline, PipelineLayout, ShaderModule, ...)
  └── :shader           (ShaderCompiler)

Vague 6 (dépend de Vague 5) :
  ├── :model            (Scene, Mesh, Material/...)
  ├── :renderpass       (Subpass, ScreenSpacePass, ...)
  ├── :raytracing       (BLAS, TLAS, ...)
  └── :random           (NoiseTexture, RandomSamplingBuffer)

Vague 7 :
  └── vw (module principal qui ré-exporte toutes les partitions)
```

---

## 7. Phase 1 — Configuration CMake

### 7.1. Mise à jour du CMakeLists.txt racine

```cmake
# CMakeLists.txt (racine)
cmake_minimum_required(VERSION 3.30)  # ← CHANGÉ de 3.25

project(VulkanWrapper LANGUAGES CXX)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS True)

option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

add_library(coverage_options INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(coverage_options INTERFACE
        -fprofile-instr-generate
        -fcoverage-mapping
    )
    target_link_options(coverage_options INTERFACE
        -fprofile-instr-generate
    )
elseif(ENABLE_COVERAGE)
    message(WARNING "Code coverage is only supported with Clang compiler")
endif()

enable_testing()
add_subdirectory(VulkanWrapper)
add_subdirectory(examples)
```

### 7.2. Mise à jour du CMakeLists.txt de la library

```cmake
# VulkanWrapper/CMakeLists.txt
cmake_minimum_required(VERSION 3.30)

include(FetchContent)

find_package(glm REQUIRED COMPONENTS glm)
find_package(assimp REQUIRED COMPONENTS assimp)
find_package(SDL3 REQUIRED COMPONENTS SDL3)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)

add_library(VulkanWrapperCoreLibrary SHARED)

# ━━━ C++23 avec support des modules ━━━
target_compile_features(VulkanWrapperCoreLibrary PUBLIC cxx_std_23)

# ━━━ Module interface units (PUBLIC) ━━━
target_sources(VulkanWrapperCoreLibrary
    PUBLIC
        FILE_SET modules TYPE CXX_MODULES FILES
            # Module principal
            src/vw.cppm

            # Partitions (interface)
            src/VulkanWrapper/Utils/vw-utils.cppm
            src/VulkanWrapper/Vulkan/vw-vulkan.cppm
            src/VulkanWrapper/Memory/vw-memory.cppm
            src/VulkanWrapper/Image/vw-image.cppm
            src/VulkanWrapper/Command/vw-command.cppm
            src/VulkanWrapper/Synchronization/vw-sync.cppm
            src/VulkanWrapper/Descriptors/vw-descriptors.cppm
            src/VulkanWrapper/Pipeline/vw-pipeline.cppm
            src/VulkanWrapper/Shader/vw-shader.cppm
            src/VulkanWrapper/Model/vw-model.cppm
            src/VulkanWrapper/RenderPass/vw-renderpass.cppm
            src/VulkanWrapper/RayTracing/vw-raytracing.cppm
            src/VulkanWrapper/Random/vw-random.cppm
            src/VulkanWrapper/Window/vw-window.cppm
)

# ━━━ Module implementation units (PRIVATE) ━━━
target_sources(VulkanWrapperCoreLibrary
    PRIVATE
        src/VulkanWrapper/Utils/Error.cpp
        src/VulkanWrapper/Vulkan/Instance.cpp
        src/VulkanWrapper/Vulkan/Device.cpp
        # ... tous les .cpp existants
)

target_compile_definitions(VulkanWrapperCoreLibrary PRIVATE VW_LIB)

target_link_libraries(VulkanWrapperCoreLibrary
    PUBLIC
        glm::glm
        assimp::assimp
        SDL3::SDL3
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        SDL3_image::SDL3_image
    PRIVATE
        $<$<BOOL:${ENABLE_COVERAGE}>:coverage_options>
        unofficial::shaderc::shaderc
)

# ━━━ PCH supprimé (remplacé par les modules) ━━━
# SUPPRIMÉ: target_precompile_headers(...)

add_library(VulkanWrapper::VW ALIAS VulkanWrapperCoreLibrary)

add_subdirectory(tests)
```

### 7.3. Commande de build

```bash
# Le générateur Ninja est OBLIGATOIRE
cmake -B build -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build build
```

---

## 8. Phase 2 — Module wrapper pour les dépendances tierces

Le problème majeur : **vulkan.hpp, GLM, SDL, VMA, assimp** ne sont pas des
modules. Il faut les consommer via le **global module fragment**.

### Approche recommandée : un header « pont » interne

On garde un header interne (non exporté) qui centralise les `#include` et les
`#define` nécessaires pour les dépendances tierces. Chaque partition qui a
besoin de ces dépendances l'inclura dans son global module fragment.

```cpp
// src/VulkanWrapper/internal/third_party_includes.h
// ⚠️  CE FICHIER N'EST PAS UN MODULE — c'est un header legacy
// Il est inclus UNIQUEMENT dans les global module fragments des partitions

#pragma once

// ━━━ Vulkan Configuration ━━━
namespace vk::detail {
class DispatchLoaderDynamic;
}

namespace vw {
const vk::detail::DispatchLoaderDynamic &DefaultDispatcher();
}

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS 1
#define VULKAN_HPP_ASSERT_ON_RESULT

#ifndef VW_LIB
#define VULKAN_HPP_DEFAULT_DISPATCHER vw::DefaultDispatcher()
#endif

// ━━━ GLM Configuration ━━━
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// ━━━ Includes ━━━
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_mem_alloc.h>
```

Chaque partition l'utilise ainsi :

```cpp
module;                                         // global module fragment
#include "internal/third_party_includes.h"      // headers legacy
export module vw:image;                         // début du module
import std;                                     // STL via module
import :utils;                                  // partition sœur
// ...
```

---

## 9. Phase 3 — Migration module par module

### 9.1. Exemple complet : partition `:utils`

Cette partition regroupe `ObjectWithHandle`, `Error`, `Algos`, `Alignment`.

**Fichier interface** — `src/VulkanWrapper/Utils/vw-utils.cppm` :

```cpp
module;

// Global module fragment : headers legacy nécessaires
#include "internal/third_party_includes.h"

export module vw:utils;

import std;

// ━━━━━ Error (exception hierarchy) ━━━━━
export namespace vw {

class Exception : public std::exception {
  public:
    explicit Exception(std::string message) noexcept;
    [[nodiscard]] const char *what() const noexcept override;
  private:
    std::string m_message;
};

class VulkanException : public Exception {
  public:
    VulkanException(vk::Result result, std::string context);
};

class VMAException : public Exception {
  public:
    VMAException(VkResult result, std::string context);
};

// ... autres exceptions (SDLException, FileException, etc.)

auto check_vk(auto result, std::string_view context) {
    // ... template, doit être dans l'interface
}

void check_vma(VkResult result, std::string_view context);
void check_sdl(bool result, std::string_view context);
void check_sdl(const void *ptr, std::string_view context);

} // namespace vw

// ━━━━━ ObjectWithHandle ━━━━━
export namespace vw {

template <typename T> class ObjectWithHandle {
  public:
    ObjectWithHandle(auto handle) noexcept
        : m_handle{std::move(handle)} {}

    auto handle() const noexcept {
        if constexpr (sizeof(T) > sizeof(void *))
            return *m_handle;
        else
            return m_handle;
    }

  private:
    T m_handle;
};

template <typename UniqueHandle>
using ObjectWithUniqueHandle = ObjectWithHandle<UniqueHandle>;

constexpr auto to_handle = std::views::transform([](const auto &x) {
    if constexpr (requires { x.handle(); })
        return x.handle();
    else
        return x->handle();
});

} // namespace vw

// ━━━━━ Algos ━━━━━
export namespace vw {

std::optional<int> index_of(const auto &range, const auto &object) {
    auto it = std::find(std::begin(range), std::end(range), object);
    if (it == std::end(range))
        return std::nullopt;
    return std::distance(std::begin(range), it);
}

std::optional<int> index_if(const auto &range, const auto &predicate) {
    auto it =
        std::find_if(std::begin(range), std::end(range), predicate);
    if (it == std::end(range))
        return std::nullopt;
    return std::distance(std::begin(range), it);
}

template <template <typename... Ts> typename Container> struct to_t {};

template <template <typename... Ts> typename Container>
constexpr to_t<Container> to{};

template <typename Range, template <typename... Ts> typename Container>
auto operator|(Range &&range, to_t<Container>) {
    return Container(std::forward<Range>(range).begin(),
                     std::forward<Range>(range).end());
}

template <typename T> struct construct_t {};

template <typename T> constexpr construct_t<T> construct{};

template <typename Range, typename T>
auto operator|(Range &&range, construct_t<T>) {
    auto constructor = [](const auto &x) {
        if constexpr (requires { T{x}; })
            return T{x};
        else
            return std::apply(
                [](auto &&...xs) {
                    return T{std::forward<decltype(xs)>(xs)...};
                },
                x);
    };
    return std::forward<Range>(range) |
           std::views::transform(constructor);
}

} // namespace vw

// ━━━━━ Alignment ━━━━━
export namespace vw {
// ... contenu de Alignment.h
}

// ━━━━━ Types forts (anciennement dans 3rd_party.h) ━━━━━
export namespace vw {

enum ApiVersion {
    e10 = vk::ApiVersion10,
    e11 = vk::ApiVersion11,
    e12 = vk::ApiVersion12,
    e13 = vk::ApiVersion13
};

enum class Width {};
enum class Height {};
enum class Depth {};
enum class MipLevel {};

} // namespace vw

// ━━━━━ Helper global (anciennement dans 3rd_party.h) ━━━━━
export template <typename... Fs> struct overloaded : Fs... {
    using Fs::operator()...;
};
```

**Fichier implémentation** — `src/VulkanWrapper/Utils/Error.cpp` :

```cpp
module;

#include "internal/third_party_includes.h"

module vw:utils;  // ← implémentation de la partition :utils

// Pas besoin de "import std;" ici : l'interface l'a déjà importé

namespace vw {

Exception::Exception(std::string message) noexcept
    : m_message(std::move(message)) {}

const char *Exception::what() const noexcept {
    return m_message.c_str();
}

VulkanException::VulkanException(vk::Result result, std::string context)
    : Exception(std::move(context) + ": " + vk::to_string(result)) {}

VMAException::VMAException(VkResult result, std::string context)
    : Exception(std::move(context) + ": VMA error " +
                std::to_string(result)) {}

void check_vma(VkResult result, std::string_view context) {
    if (result != VK_SUCCESS)
        throw VMAException(result, std::string(context));
}

void check_sdl(bool result, std::string_view context) {
    if (!result) throw SDLException(std::string(context));
}

void check_sdl(const void *ptr, std::string_view context) {
    if (!ptr) throw SDLException(std::string(context));
}

} // namespace vw
```

### 9.2. Exemple : partition `:vulkan`

```cpp
// src/VulkanWrapper/Vulkan/vw-vulkan.cppm
module;

#include "internal/third_party_includes.h"

export module vw:vulkan;

import std;
import :utils;  // pour ObjectWithHandle, Error, ApiVersion, etc.

export namespace vw {

// ━━━ Forward declarations (remplace fwd.h) ━━━
// Plus nécessaire ! Les types sont disponibles via import :partition

// ━━━ Instance ━━━
class Instance {
    friend class InstanceBuilder;

  public:
    Instance(const Instance &) = delete;
    Instance(Instance &&) = delete;
    Instance &operator=(Instance &&) = delete;
    Instance &operator=(const Instance &) = delete;

    [[nodiscard]] vk::Instance handle() const noexcept;
    [[nodiscard]] DeviceFinder findGpu() const noexcept;

  private:
    Instance(vk::UniqueInstance instance,
             vk::UniqueDebugUtilsMessengerEXT debugMessenger,
             std::span<const char *> extensions,
             ApiVersion apiVersion) noexcept;

    struct Impl {
        vk::UniqueInstance instance;
        vk::UniqueDebugUtilsMessengerEXT debugMessenger;
        std::vector<const char *> extensions;
        ApiVersion version;

        Impl(vk::UniqueInstance inst,
             vk::UniqueDebugUtilsMessengerEXT debugMsgr,
             std::span<const char *> exts,
             ApiVersion apiVersion) noexcept;

        Impl(const Impl &) = delete;
        Impl &operator=(const Impl &) = delete;
        Impl(Impl &&) = delete;
        Impl &operator=(Impl &&) = delete;
    };

    std::shared_ptr<Impl> m_impl;
};

class InstanceBuilder {
  public:
    InstanceBuilder &addPortability();
    InstanceBuilder &addExtension(const char *extension);
    InstanceBuilder &addExtensions(
        std::span<const char *const> extensions);
    InstanceBuilder &setDebug();
    InstanceBuilder &setApiVersion(ApiVersion version);
    std::shared_ptr<Instance> build();

  private:
    vk::InstanceCreateFlags m_flags;
    std::vector<const char *> m_extensions;
    std::vector<const char *> m_layers;
    bool m_debug = false;
    ApiVersion m_version = ApiVersion::e10;
};

// ━━━ Device, DeviceFinder, Queue, etc. ━━━
// ... (même approche)

} // namespace vw
```

### 9.3. Exemple : partition `:memory` (avec templates complexes)

```cpp
// src/VulkanWrapper/Memory/vw-memory.cppm
module;

#include "internal/third_party_includes.h"

export module vw:memory;

import std;
import :utils;
import :vulkan;

// ━━━ BufferUsage constants ━━━
export namespace vw {

constexpr VkBufferUsageFlags2 VertexBufferUsage = VkBufferUsageFlags2{
    vk::BufferUsageFlagBits2::eVertexBuffer |
    vk::BufferUsageFlagBits2::eTransferDst |
    vk::BufferUsageFlagBits2::eShaderDeviceAddress |
    vk::BufferUsageFlagBits2::
        eAccelerationStructureBuildInputReadOnlyKHR};

// ... autres constantes

} // namespace vw

// ━━━ BufferBase ━━━
export namespace vw {

class BufferBase : public ObjectWithHandle<vk::Buffer> {
  public:
    BufferBase(std::shared_ptr<const Device> device,
               std::shared_ptr<const Allocator> allocator,
               vk::Buffer buffer, VmaAllocation allocation,
               VkDeviceSize size);

    // ... (identique à l'existant)
    ~BufferBase();

  private:
    struct Data {
        std::shared_ptr<const Device> m_device;
        std::shared_ptr<const Allocator> m_allocator;
        VmaAllocation m_allocation;
        VkDeviceSize m_size_in_bytes;
    };
    std::unique_ptr<Data> m_data;
};

// ━━━ Buffer<T, HostVisible, Flags> ━━━
// Template : DOIT être dans l'interface unit
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer : public BufferBase {
  public:
    using type = T;
    static constexpr auto host_visible = HostVisible;
    static constexpr auto flags = vk::BufferUsageFlags(Flags);

    static consteval bool does_support(vk::BufferUsageFlags usage) {
        return (vk::BufferUsageFlags(flags) & usage) == usage;
    }

    Buffer(BufferBase &&bufferBase)
        : BufferBase(std::move(bufferBase)) {}

    void write(std::span<const T> data, std::size_t offset)
        requires(HostVisible)
    {
        BufferBase::write_bytes(data.data(), data.size_bytes(),
                                offset * sizeof(T));
    }

    void write(const T &element, std::size_t offset)
        requires(HostVisible)
    {
        BufferBase::write_bytes(&element, sizeof(T),
                                offset * sizeof(T));
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return size_bytes() / sizeof(T);
    }

    [[nodiscard]] std::vector<T>
    read_as_vector(std::size_t offset, std::size_t count) const
        requires(HostVisible)
    {
        auto bytes = BufferBase::read_bytes(offset * sizeof(T),
                                             count * sizeof(T));
        std::vector<T> result(count);
        std::memcpy(result.data(), bytes.data(), bytes.size());
        return result;
    }
};

using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;

} // namespace vw

// ━━━ create_buffer, allocate_vertex_buffer ━━━
export namespace vw {

template <typename T, bool HostVisible, VkBufferUsageFlags Usage>
Buffer<T, HostVisible, Usage>
create_buffer(const Allocator &allocator, std::size_t count) {
    // ... implémentation (template, doit être ici)
}

} // namespace vw
```

---

## 10. Phase 4 — Module principal et partitions

### 10.1. Module principal : `vw.cppm`

Le module principal ré-exporte toutes les partitions. C'est le point d'entrée
unique pour les consommateurs :

```cpp
// src/vw.cppm
export module vw;

// Ré-exporter toutes les partitions
export import :utils;
export import :vulkan;
export import :memory;
export import :image;
export import :command;
export import :sync;
export import :descriptors;
export import :pipeline;
export import :shader;
export import :model;
export import :renderpass;
export import :raytracing;
export import :random;
export import :window;
```

### 10.2. Consommation par le code utilisateur

**Avant** (headers) :

```cpp
#include <VulkanWrapper/3rd_party.h>
#include <VulkanWrapper/Vulkan/Instance.h>
#include <VulkanWrapper/Vulkan/Device.h>
#include <VulkanWrapper/Memory/Allocator.h>
#include <VulkanWrapper/Memory/Buffer.h>
#include <VulkanWrapper/Image/Image.h>
// ... 15 includes

int main() {
    auto instance = vw::InstanceBuilder()
        .setDebug()
        .setApiVersion(vw::ApiVersion::e13)
        .build();
    // ...
}
```

**Après** (module) :

```cpp
import vw;     // ← un seul import, TOUT est disponible

int main() {
    auto instance = vw::InstanceBuilder()
        .setDebug()
        .setApiVersion(vw::ApiVersion::e13)
        .build();
    // ...
}
```

On peut aussi importer des partitions spécifiques si on veut un import
plus fin (utile pour les tests par exemple), mais seulement depuis
**l'intérieur** du module `vw`. Les consommateurs externes ne voient que
`import vw;`.

---

## 11. Phase 5 — Migration des tests et exemples

### 11.1. Tests

Les tests deviennent beaucoup plus simples :

**Avant** :

```cpp
// tests/Memory/BufferTests.cpp
#include <VulkanWrapper/Memory/Buffer.h>
#include <VulkanWrapper/Memory/AllocateBufferUtils.h>
#include <VulkanWrapper/Memory/Allocator.h>
#include "utils/create_gpu.hpp"
#include <gtest/gtest.h>
```

**Après** :

```cpp
// tests/Memory/BufferTests.cpp
import vw;
#include "utils/create_gpu.hpp"  // reste un header (singleton de test)
#include <gtest/gtest.h>          // GTest n'a pas de module (pour le moment)
```

### 11.2. CMake des tests

```cmake
# VulkanWrapper/tests/CMakeLists.txt
find_package(GTest CONFIG REQUIRED)

add_library(TestUtils STATIC utils/create_gpu.cpp)
target_link_libraries(TestUtils PUBLIC VulkanWrapperCoreLibrary)
target_include_directories(TestUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(MemoryTests
    Memory/IntervalTests.cpp
    Memory/BufferTests.cpp
    # ...
)

target_link_libraries(MemoryTests PRIVATE
    TestUtils
    VulkanWrapperCoreLibrary    # ← donne accès à "import vw;"
    GTest::gtest
    GTest::gtest_main
)
```

Pas de changement CMake significatif pour les tests — CMake propage
automatiquement les modules via `target_link_libraries`.

### 11.3. Exemples

```cpp
// examples/Advanced/main.cpp
import vw;                      // ← remplace les 15+ includes
#include "Application.h"        // header local de l'exemple
#include "DeferredRenderingManager.h"
```

---

## 12. Gestion des templates et du code header-only

### Règle fondamentale

> Tout code **template** et **inline/constexpr** doit être dans le **module
> interface unit** (`.cppm`), pas dans l'implementation unit (`.cpp`).

C'est le même principe qu'avec les headers : les templates doivent être
visibles au point d'instanciation.

### Dans VulkanWrapper, cela concerne :

| Fichier actuel | Contenu template | Action |
|---|---|---|
| `Buffer.h` | `Buffer<T, HostVisible, Flags>` | Entièrement dans `:memory` interface |
| `ObjectWithHandle.h` | `ObjectWithHandle<T>`, `to_handle` | Entièrement dans `:utils` interface |
| `Subpass.h` | `Subpass<SlotEnum>` | Entièrement dans `:renderpass` interface |
| `ScreenSpacePass.h` | `ScreenSpacePass<SlotEnum>` | Entièrement dans `:renderpass` interface |
| `Algos.h` | `index_of`, `index_if`, `to`, `construct` | Entièrement dans `:utils` interface |
| `Pipeline.h` | `GraphicsPipelineBuilder::add_vertex_binding<V>()` | Template dans interface, reste dans impl |
| `Vertex.h` | Concept `Vertex` | Dans `:descriptors` interface |

### Patron pour les classes mixtes (template + non-template)

```cpp
// Dans le .cppm (interface) :
export namespace vw {

class GraphicsPipelineBuilder {
  public:
    // Méthodes non-template : déclaration seule
    GraphicsPipelineBuilder &add_dynamic_state(vk::DynamicState state);
    std::shared_ptr<const Pipeline> build();

    // Méthode template : implémentation complète ici
    template <Vertex V>
    GraphicsPipelineBuilder &add_vertex_binding() {
        const auto binding = m_input_binding_descriptions.size();
        // ... (code complet)
        return *this;
    }

  private:
    // ...
};

} // namespace vw
```

```cpp
// Dans le .cpp (implémentation) :
module vw:pipeline;

namespace vw {

GraphicsPipelineBuilder &
GraphicsPipelineBuilder::add_dynamic_state(vk::DynamicState state) {
    m_dynamicStates.push_back(state);
    return *this;
}

std::shared_ptr<const Pipeline> GraphicsPipelineBuilder::build() {
    // ... implémentation complète
}

} // namespace vw
```

---

## 13. Pièges courants et solutions

### 13.1. Les macros ne traversent pas les frontières de module

**Problème** : Le projet utilise `VULKAN_HPP_DISPATCH_LOADER_DYNAMIC`,
`GLM_FORCE_RADIANS`, etc. Ces macros doivent être définies **avant** d'inclure
les headers correspondants.

**Solution** : Le global module fragment.

```cpp
module;

// Les macros et les #include sont ici, AVANT la déclaration de module
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

export module vw:vulkan;  // les macros ne fuient pas au-delà
```

Concrètement, le header pont `internal/third_party_includes.h` résout ce
problème de manière centralisée (voir Phase 2).

### 13.2. Les entités du global module fragment ont un linkage spécial

Les types inclus via le global module fragment (comme `vk::Instance`,
`glm::mat4`) sont **attachés au module global**, pas au module nommé. Cela
signifie qu'ils sont les mêmes types partout — pas de problème d'ODR.

Cependant, **attention** : si deux partitions incluent le même header dans leur
global module fragment, le compilateur doit vérifier la cohérence. C'est
pourquoi le header pont centralisé est important — il garantit les mêmes
`#define` partout.

### 13.3. `inline` et `constexpr` dans les modules

Dans un header, une fonction définie dans la classe est implicitement `inline`.
**Dans un module, ce n'est pas le cas.**

```cpp
export module vw:utils;

export namespace vw {

class Foo {
  public:
    // ⚠️  Dans un module, ce n'est PAS inline automatiquement !
    int bar() { return 42; }

    // Si vous voulez qu'elle soit inline, soyez explicite :
    inline int baz() { return 42; }
};

} // namespace vw
```

Pour les fonctions template et constexpr, ce n'est pas un problème car elles
sont implicitement inline.

**Recommandation** : pour les petites fonctions non-template que vous voulez
inline (getters, etc.), ajoutez `inline` explicitement dans le module.

### 13.4. Visibilité par défaut : rien n'est exporté

Contrairement aux headers où tout ce qui est défini est visible, dans un
module, **rien n'est exporté par défaut**. Il faut explicitement marquer chaque
entité avec `export` :

```cpp
export module vw:utils;

// ❌ INVISIBLE de l'extérieur :
namespace vw::detail {
    int helper() { return 42; }
}

// ✅ VISIBLE de l'extérieur :
export namespace vw {
    int public_function();
}
```

C'est un **avantage** : vous pouvez enfin avoir des détails d'implémentation
vraiment privés, sans recourir au pattern Pimpl ou aux namespaces `detail`.

### 13.5. Forward declarations inter-modules

Les forward declarations classiques ne fonctionnent pas à travers les
frontières de module. Si la partition `:memory` a besoin de connaître `Device`
(défini dans `:vulkan`), elle doit faire `import :vulkan;`.

**C'est une simplification** : le fichier `fwd.h` actuel devient inutile.
Les dépendances sont explicitées par les `import`.

### 13.6. Ordre de compilation

Les modules imposent un **ordre de compilation strict** : un module doit être
compilé avant les modules qui l'importent. C'est pourquoi Ninja est obligatoire
— il gère ce graphe de dépendances.

Avec le graphe du projet :

```
:utils → :vulkan → :memory → :image → :pipeline → ...
```

Ninja compilera automatiquement dans le bon ordre.

### 13.7. Les classes template exportées doivent être complètes

Si vous exportez une classe template, **toute son implémentation doit être dans
l'interface unit**. Vous ne pouvez pas séparer la déclaration dans le `.cppm`
et l'implémentation dans le `.cpp` pour les templates.

C'est identique au modèle header/source actuel — les templates sont déjà dans
les headers.

### 13.8. `std::` types dans les interfaces exportées

Les types de la STL (`std::vector`, `std::shared_ptr`, etc.) dans les
signatures exportées fonctionnent correctement avec `import std;`. Le
compilateur sait que `std::vector` est le même type partout.

```cpp
export module vw:vulkan;
import std;

export namespace vw {
    class Instance {
        // ✅ std::shared_ptr dans une interface exportée : OK
        std::shared_ptr<Impl> m_impl;
    };
}
```

---

## 14. Coexistence headers/modules pendant la transition

La migration ne se fait pas en un jour. Voici comment faire coexister les deux
systèmes.

### 14.1. Approche « module wrapper »

On garde les headers existants intacts et on crée des modules qui les
incluent :

```cpp
// src/VulkanWrapper/Utils/vw-utils.cppm
module;

// Inclure les headers legacy dans le global module fragment
#include "VulkanWrapper/Utils/ObjectWithHandle.h"
#include "VulkanWrapper/Utils/Error.h"
#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Utils/Alignment.h"

export module vw:utils;

// Ré-exporter tout ce qui était dans les headers
export namespace vw {
    using vw::ObjectWithHandle;
    using vw::ObjectWithUniqueHandle;
    using vw::to_handle;
    using vw::Exception;
    using vw::VulkanException;
    // ... etc.
}
```

**Avantages** :

- Migration progressive : les headers continuent de fonctionner
- Les consommateurs peuvent utiliser `import vw;` OU `#include` pendant la
  transition
- Risque minimal : pas de réécriture du code existant

**Inconvénients** :

- Ne bénéficie pas pleinement de l'isolation des modules (les headers sont
  toujours parsés textuellement dans le global module fragment)
- Solution temporaire — l'objectif final est de supprimer les headers

### 14.2. Migration progressive recommandée

```
Étape 1 : Module wrappers (rapide, faible risque)
  → Créer les .cppm qui incluent les headers existants
  → Les tests/exemples commencent à utiliser "import vw;"
  → Les headers restent inchangés
  → Valider que tout compile et passe les tests

Étape 2 : Déplacer le code dans les modules (une partition à la fois)
  → Commencer par :utils (la moins de dépendances)
  → Déplacer le code du header vers le .cppm
  → Supprimer le header quand la partition est autonome
  → Valider après chaque partition

Étape 3 : Nettoyer
  → Supprimer fwd.h (les imports remplacent les forward declarations)
  → Supprimer 3rd_party.h (remplacé par le header pont interne)
  → Supprimer le PCH
  → Supprimer les headers vides
```

---

## 15. Impact sur le PCH existant

### Situation actuelle

Le PCH est `3rd_party.h`, qui contient :
- Les macros Vulkan (`VULKAN_HPP_DISPATCH_LOADER_DYNAMIC`, etc.)
- Les includes Vulkan, GLM, et STL
- Les types forts (`Width`, `Height`, etc.)
- Le helper `overloaded`
- L'enum `ApiVersion`

```cmake
# Actuel
target_precompile_headers(VulkanWrapperCoreLibrary
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanWrapper/3rd_party.h)
```

### Avec les modules

Le PCH devient **inutile et contre-productif**. Les modules C++20 sont compilés
une seule fois en BMI, ce qui remplit exactement le même rôle que le PCH mais
en mieux :

| Aspect | PCH | Module BMI |
|--------|-----|------------|
| Compilé une fois | Oui | Oui |
| Partagé entre TU | Oui | Oui |
| Isolation des macros | Non | Oui |
| Standard C++ | Non (extension) | Oui |
| Compatible modules | Non | Oui |

**Action** : supprimer la ligne `target_precompile_headers(...)` du CMake. Le
contenu de `3rd_party.h` est distribué entre :
- Le header pont interne (`internal/third_party_includes.h`) pour les includes
  Vulkan/GLM
- La partition `:utils` pour `ApiVersion`, `Width`, `Height`, `overloaded`

---

## 16. Checklist de migration complète

### Préparation

- [ ] Mettre à jour CMake vers 3.30+
- [ ] Installer Ninja 1.11+
- [ ] Vérifier le support modules de Clang (`clang++ --version` ≥ 18)
- [ ] Vérifier que `import std;` fonctionne (test trivial)
- [ ] Créer une branche de migration

### Phase 1 — Infrastructure

- [ ] Mettre à jour `cmake_minimum_required(VERSION 3.30)`
- [ ] Configurer avec `-G Ninja`
- [ ] Créer `src/VulkanWrapper/internal/third_party_includes.h`
- [ ] Vérifier que le build classique fonctionne encore

### Phase 2 — Module wrappers (coexistence)

- [ ] Créer `src/vw.cppm` (module principal)
- [ ] Créer les `.cppm` de chaque partition (avec `#include` des headers)
- [ ] Enregistrer les `.cppm` dans CMake (`FILE_SET CXX_MODULES`)
- [ ] Vérifier que `import vw;` fonctionne dans un test simple
- [ ] Migrer un test pour utiliser `import vw;` au lieu des includes
- [ ] Vérifier que tous les tests passent

### Phase 3 — Migration par vagues

Pour chaque vague (voir section 6) :

- [ ] Déplacer le code des headers vers le `.cppm` de la partition
- [ ] Convertir les `#include` internes en `import :partition;`
- [ ] Ajuster la visibilité avec `export`
- [ ] Convertir les `.cpp` en module implementation units (`module vw:xxx;`)
- [ ] Vérifier la compilation
- [ ] Exécuter tous les tests
- [ ] Supprimer les headers devenus vides

Ordre des vagues :
- [ ] Vague 1 : `:utils`, `:window`
- [ ] Vague 2 : `:vulkan`
- [ ] Vague 3 : `:command`, `:sync`, `:memory`
- [ ] Vague 4 : `:image`, `:descriptors`
- [ ] Vague 5 : `:pipeline`, `:shader`
- [ ] Vague 6 : `:model`, `:renderpass`, `:raytracing`, `:random`
- [ ] Vague 7 : Module principal `vw`

### Phase 4 — Nettoyage

- [ ] Supprimer `3rd_party.h`
- [ ] Supprimer `fwd.h`
- [ ] Supprimer `target_precompile_headers(...)` du CMake
- [ ] Supprimer les `add_subdirectory` du `include/` CMake (plus de headers)
- [ ] Migrer les exemples vers `import vw;`
- [ ] Migrer tous les tests vers `import vw;`
- [ ] Supprimer le dossier `include/VulkanWrapper/` entier
- [ ] Mettre à jour CLAUDE.md

### Phase 5 — Validation finale

- [ ] Build propre (`cmake --build build --clean-first`)
- [ ] Tous les tests passent
- [ ] Les exemples fonctionnent
- [ ] Pas de régression de temps de build (devrait être plus rapide)
- [ ] Pas de warning de compilation

---

## Annexe A — Arborescence cible après migration

```
VulkanWrapper/
├── src/
│   ├── vw.cppm                                    # Module principal
│   └── VulkanWrapper/
│       ├── internal/
│       │   └── third_party_includes.h             # Header pont (non-module)
│       ├── Utils/
│       │   ├── vw-utils.cppm                      # Partition interface
│       │   └── Error.cpp                          # Implementation unit
│       ├── Vulkan/
│       │   ├── vw-vulkan.cppm
│       │   ├── Instance.cpp
│       │   ├── Device.cpp
│       │   ├── DeviceFinder.cpp
│       │   ├── Queue.cpp
│       │   ├── PhysicalDevice.cpp
│       │   ├── PresentQueue.cpp
│       │   ├── Swapchain.cpp
│       │   └── Surface.cpp
│       ├── Memory/
│       │   ├── vw-memory.cppm
│       │   ├── Allocator.cpp
│       │   ├── Buffer.cpp
│       │   ├── Barrier.cpp
│       │   ├── Transfer.cpp
│       │   ├── StagingBufferManager.cpp
│       │   ├── UniformBufferAllocator.cpp
│       │   ├── IntervalSet.cpp
│       │   ├── Interval.cpp
│       │   └── vma.cpp
│       ├── Image/
│       │   ├── vw-image.cppm
│       │   ├── Image.cpp
│       │   ├── ImageView.cpp
│       │   ├── ImageLoader.cpp
│       │   ├── Sampler.cpp
│       │   ├── CombinedImage.cpp
│       │   └── Mipmap.cpp
│       ├── Command/
│       │   ├── vw-command.cppm
│       │   ├── CommandPool.cpp
│       │   └── CommandBuffer.cpp
│       ├── Synchronization/
│       │   ├── vw-sync.cppm
│       │   ├── Fence.cpp
│       │   ├── Semaphore.cpp
│       │   └── ResourceTracker.cpp
│       ├── Descriptors/
│       │   ├── vw-descriptors.cppm
│       │   ├── DescriptorSet.cpp
│       │   ├── DescriptorSetLayout.cpp
│       │   ├── DescriptorPool.cpp
│       │   └── DescriptorAllocator.cpp
│       ├── Pipeline/
│       │   ├── vw-pipeline.cppm
│       │   ├── Pipeline.cpp
│       │   ├── PipelineLayout.cpp
│       │   ├── ShaderModule.cpp
│       │   ├── ComputePipeline.cpp
│       │   └── MeshRenderer.cpp
│       ├── Shader/
│       │   ├── vw-shader.cppm
│       │   └── ShaderCompiler.cpp
│       ├── Model/
│       │   ├── vw-model.cppm
│       │   ├── Mesh.cpp
│       │   ├── MeshManager.cpp
│       │   ├── Importer.cpp
│       │   ├── Internal/
│       │   │   ├── MeshInfo.cpp
│       │   │   └── MaterialInfo.cpp
│       │   └── Material/
│       │       ├── Material.cpp
│       │       ├── BindlessMaterialManager.cpp
│       │       ├── BindlessTextureManager.cpp
│       │       ├── ColoredMaterialHandler.cpp
│       │       ├── TexturedMaterialHandler.cpp
│       │       └── EmissiveTexturedMaterialHandler.cpp
│       ├── RenderPass/
│       │   ├── vw-renderpass.cppm
│       │   ├── ScreenSpacePass.cpp
│       │   ├── SkyPass.cpp
│       │   ├── SkyParameters.cpp
│       │   ├── ToneMappingPass.cpp
│       │   └── IndirectLightPass.cpp
│       ├── RayTracing/
│       │   ├── vw-raytracing.cppm
│       │   ├── BottomLevelAccelerationStructure.cpp
│       │   ├── TopLevelAccelerationStructure.cpp
│       │   ├── RayTracedScene.cpp
│       │   ├── RayTracingPipeline.cpp
│       │   └── ShaderBindingTable.cpp
│       ├── Random/
│       │   ├── vw-random.cppm
│       │   ├── NoiseTexture.cpp
│       │   └── RandomSamplingBuffer.cpp
│       └── Window/
│           ├── vw-window.cppm
│           ├── SDL_Initializer.cpp
│           └── Window.cpp
├── tests/                              # Inchangé (utilise import vw;)
├── Shaders/                            # Inchangé
└── CMakeLists.txt                      # Mis à jour (FILE_SET CXX_MODULES)
```

**Note** : le dossier `include/` disparaît entièrement. Les modules interface
units (`.cppm`) sont dans `src/` et CMake les rend disponibles via `FILE_SET
CXX_MODULES PUBLIC`.

---

## Annexe B — Référence rapide de syntaxe

```cpp
// ━━━ Créer un module ━━━
export module nom;                  // Module interface unit (.cppm)
module nom;                         // Module implementation unit (.cpp)

// ━━━ Partitions ━━━
export module nom:partition;        // Partition interface
module nom:partition;               // Partition implementation
import :partition;                  // Import intra-module
export import :partition;           // Ré-export depuis le module principal

// ━━━ Exporter ━━━
export int foo();                   // Fonction
export class Bar {};                // Classe
export namespace ns { /* ... */ }   // Tout le namespace
export { int a; void b(); }        // Bloc d'export
export using X = int;               // Type alias
export template<typename T>         // Template
    class Baz {};
export enum class E { A, B };       // Enum

// ━━━ Importer ━━━
import nom;                         // Module nommé
import std;                         // La STL entière (C++23)
import :partition;                  // Partition sœur

// ━━━ Global module fragment ━━━
module;                             // Début
#include <legacy_header.h>          // Headers non-modules
export module nom;                  // Fin du fragment
```

---

## Annexe C — Diagnostic des erreurs courantes

| Erreur | Cause probable | Solution |
|--------|---------------|----------|
| `error: import of module 'X' appears after first declaration` | `import` après du code | Placer tous les `import` en haut |
| `error: redefinition of 'Y'` | Type défini dans le global module fragment et dans le module | Ne pas redéfinir les types du fragment |
| `error: use of undeclared identifier 'Z'` | Entité non exportée | Ajouter `export` |
| `error: module 'X' not found` | BMI pas encore compilé | Vérifier l'ordre de build (utiliser Ninja) |
| `error: import of module 'X:part' from outside module 'X'` | Import de partition depuis l'extérieur | Les partitions sont internes, utiliser `import X;` |
| `error: definition of 'f' in module 'X' cannot have internal linkage` | Fonction/variable `static` dans un module | Utiliser un namespace anonyme ou un namespace `detail` non exporté |
| Build lent / recompilation totale | Changement dans un `.cppm` interface | Normal : les dépendants recompilent. Minimiser les changements d'interface |

---

## Annexe D — FAQ

**Q : Est-ce que je peux utiliser `#include` et `import` en même temps ?**
Oui, pendant la transition. Un fichier source peut mélanger les deux. Mais
l'objectif est d'éliminer progressivement les `#include`.

**Q : Et si une dépendance (GTest, SDL, Vulkan) ne supporte pas les modules ?**
On les inclut dans le global module fragment. C'est exactement pour ça qu'il
existe. Quand ces bibliothèques fourniront des modules, on remplacera le
`#include` par un `import`.

**Q : Les modules sont-ils plus lents à compiler la première fois ?**
La première compilation est similaire. C'est la **compilation incrémentale**
qui est drastiquement plus rapide : seuls les modules modifiés sont recompilés,
et les modules non modifiés ne sont même pas re-parsés.

**Q : Que se passe-t-il si je change un `.cppm` ?**
Tous les modules qui importent cette partition sont recompilés. C'est pourquoi
il est important de stabiliser les interfaces et de mettre le code
d'implémentation dans les `.cpp` (implementation units).

**Q : `import std;` importe TOUTE la STL. C'est pas lent ?**
Non. Le BMI de `std` est compilé une seule fois et partagé. L'import est quasi
instantané car le compilateur lit un fichier binaire pré-compilé, pas du texte.

**Q : Les IDE supportent-ils les modules ?**
- **CLion** : support depuis 2024.1 (avec Clang et CMake)
- **VS Code + clangd** : support partiel, en amélioration continue
- **Visual Studio** : excellent support (c'est Microsoft qui a poussé les modules)

**Q : Faut-il renommer les `.cpp` d'implémentation ?**
Non. Les fichiers `.cpp` restent `.cpp`. Seuls les **module interface units**
ont une nouvelle extension (`.cppm`, `.ixx`, ou `.mpp`). CMake se base sur le
`FILE_SET CXX_MODULES` pour les identifier, pas sur l'extension.
