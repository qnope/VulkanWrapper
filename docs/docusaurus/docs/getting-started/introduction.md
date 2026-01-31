---
sidebar_position: 1
---

# Introduction

VulkanWrapper is a modern C++23 library that provides high-level abstractions over the Vulkan graphics API while maintaining full access to Vulkan's power and flexibility.

## Why VulkanWrapper?

Vulkan is a powerful, low-level graphics API, but its verbosity can be overwhelming. VulkanWrapper addresses this by:

### 1. Reducing Boilerplate

```mermaid
graph LR
    subgraph "Without VulkanWrapper"
        A1[50+ lines] --> A2[Create Instance]
    end

    subgraph "With VulkanWrapper"
        B1[5 lines] --> B2[Create Instance]
    end
```

**Traditional Vulkan:**
```cpp
// 50+ lines of VkInstanceCreateInfo, VkApplicationInfo,
// extension enumeration, layer setup, etc.
```

**VulkanWrapper:**
```cpp
auto instance = vw::InstanceBuilder()
    .setDebug()
    .setApiVersion(vw::ApiVersion::e13)
    .build();
```

### 2. Preventing Common Errors

```mermaid
flowchart TD
    subgraph "Compile Time Safety"
        CT1[Wrong buffer usage] --> CT2[Compiler Error]
        CT3[Missing sync] --> CT4[ResourceTracker Warning]
    end
```

Type-safe buffers catch errors at compile time:

```cpp
// This won't compile - can't use vertex buffer as index buffer
using VertexBuffer = vw::Buffer<Vertex, false, VertexBufferUsage>;
using IndexBuffer = vw::Buffer<uint32_t, false, IndexBufferUsage>;

VertexBuffer vb = ...;
// cmd.bindIndexBuffer(vb);  // Compile error!
```

### 3. Automatic Resource Management

```mermaid
sequenceDiagram
    participant App
    participant ResourceTracker
    participant GPU

    App->>ResourceTracker: track(currentState)
    App->>ResourceTracker: request(newState)
    ResourceTracker->>ResourceTracker: Compute barriers
    ResourceTracker->>GPU: Insert optimal barriers
```

No more manual barrier management:

```cpp
vw::Transfer transfer;
transfer.resourceTracker().track(currentImageState);
transfer.resourceTracker().request(newImageState);
transfer.resourceTracker().flush(cmd);  // Barriers inserted automatically
```

## Core Modules

| Module | Purpose |
|--------|---------|
| **Vulkan** | Instance, device, and queue management |
| **Memory** | Allocator, buffers, and resource tracking |
| **Image** | Images, views, samplers, and loading |
| **Pipeline** | Graphics and compute pipelines |
| **Descriptors** | Descriptor sets and layouts |
| **RenderPass** | High-level rendering abstractions |
| **RayTracing** | Acceleration structures and RT pipelines |
| **Synchronization** | Fences, semaphores, and barriers |

## Design Principles

```mermaid
mindmap
  root((VulkanWrapper))
    Type Safety
      Templates
      Concepts
      Strong Types
    RAII
      Automatic Cleanup
      Exception Safety
      Scope-Based Lifetime
    Builder Pattern
      Fluent API
      Sensible Defaults
      Validation
    Modern Features
      Vulkan 1.3
      C++23
      Dynamic Rendering
```

### Type Safety

VulkanWrapper uses C++23 features to catch errors at compile time:

- **Concepts** constrain template parameters
- **Strong types** prevent unit mismatches (Width vs Height)
- **Compile-time flags** validate buffer usage

### RAII (Resource Acquisition Is Initialization)

All Vulkan resources are automatically cleaned up:

```cpp
{
    auto buffer = allocator->create_buffer<Vertex>(...);
    // Use buffer...
} // Buffer automatically destroyed here
```

### Builder Pattern

All major objects use fluent builder APIs:

```cpp
auto pipeline = vw::GraphicsPipelineBuilder(device)
    .add_shader(vertexShader)
    .add_shader(fragmentShader)
    .set_vertex_input<Vertex>()
    .set_topology(vk::PrimitiveTopology::eTriangleList)
    .build();
```

## Requirements

- **C++23** compiler (Clang 18+, GCC 13+, MSVC 2022+)
- **Vulkan 1.3** capable GPU and driver
- **CMake 3.25+** for building

## Next Steps

Continue to [Installation](./installation) to set up your development environment.
