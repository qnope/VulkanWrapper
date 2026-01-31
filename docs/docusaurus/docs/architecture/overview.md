---
sidebar_position: 1
---

# Architecture Overview

VulkanWrapper is organized into focused modules that map closely to Vulkan concepts while providing higher-level abstractions.

## Module Organization

```mermaid
graph TB
    subgraph "Application Layer"
        App[Your Application]
    end

    subgraph "High-Level Abstractions"
        RenderPass[RenderPass<br/>Subpass, ScreenSpacePass]
        Model[Model<br/>Mesh, Material]
        RT[RayTracing<br/>Scene, Pipeline]
    end

    subgraph "Core Abstractions"
        Pipeline[Pipeline<br/>Graphics, Compute]
        Descriptors[Descriptors<br/>Sets, Layouts]
        Memory[Memory<br/>Allocator, Buffers]
        ImageMod[Image<br/>Views, Samplers]
    end

    subgraph "Foundation"
        Vulkan[Vulkan<br/>Instance, Device]
        Sync[Synchronization<br/>Barriers, Fences]
        Command[Command<br/>Pool, Recorder]
    end

    App --> RenderPass
    App --> Model
    App --> RT
    App --> Pipeline
    App --> Descriptors

    RenderPass --> Pipeline
    RenderPass --> Memory
    RenderPass --> ImageMod
    Model --> Pipeline
    Model --> Descriptors
    RT --> Pipeline
    RT --> Memory

    Pipeline --> Vulkan
    Descriptors --> Vulkan
    Memory --> Vulkan
    ImageMod --> Memory
    Sync --> Vulkan
    Command --> Vulkan
```

## Core Design Patterns

### 1. Builder Pattern

All major objects use fluent builders with sensible defaults:

```mermaid
flowchart LR
    Create[Create Builder] --> Config[Configure]
    Config --> |Fluent API| Config
    Config --> Build[.build]
    Build --> Object[Vulkan Object]
```

```cpp
auto pipeline = GraphicsPipelineBuilder(device)
    .set_layout(layout)
    .add_shader(vk::ShaderStageFlagBits::eVertex, vertShader)
    .add_shader(vk::ShaderStageFlagBits::eFragment, fragShader)
    .set_vertex_input<Vertex>()
    .build();
```

### 2. RAII Resource Management

Resources are automatically cleaned up when they go out of scope:

```mermaid
sequenceDiagram
    participant Scope
    participant Resource
    participant GPU

    Scope->>Resource: Create
    Resource->>GPU: Allocate

    Note over Scope,GPU: Use resource...

    Scope->>Resource: Scope exit (destructor)
    Resource->>GPU: Free
```

### 3. Type-Safe Templates

Compile-time validation prevents common errors:

```mermaid
classDiagram
    class Buffer~T, HostVisible, Usage~ {
        +data() span~T~ [if HostVisible]
        +handle() vk::Buffer
    }

    note for Buffer "Compile-time checks:\n- T must be trivially copyable\n- Usage flags validated\n- Host access only if HostVisible"
```

### 4. Automatic Synchronization

The `ResourceTracker` generates optimal barriers:

```mermaid
sequenceDiagram
    participant App
    participant Tracker
    participant GPU

    App->>Tracker: track(current state)
    App->>Tracker: request(new state)
    App->>Tracker: flush(cmd)
    Tracker->>Tracker: Compute minimal barriers
    Tracker->>GPU: Record pipeline barriers
```

## Module Details

### Vulkan Module

Core Vulkan initialization and device management.

```mermaid
classDiagram
    class Instance {
        +findGpu() DeviceFinder
        +handle() vk::Instance
    }

    class DeviceFinder {
        +with_queue(flags) Self
        +with_dynamic_rendering() Self
        +build() Device
    }

    class Device {
        +find_queue(flags) Queue
        +wait_idle()
        +handle() vk::Device
    }

    class Queue {
        +submit2(info, fence)
        +handle() vk::Queue
    }

    Instance --> DeviceFinder
    DeviceFinder --> Device
    Device --> Queue
```

### Memory Module

Resource allocation via Vulkan Memory Allocator.

```mermaid
flowchart TB
    subgraph "Allocator"
        VMA[VMA Allocator]
    end

    subgraph "Buffer Types"
        Vertex[Vertex Buffer<br/>Device-local]
        Index[Index Buffer<br/>Device-local]
        Uniform[Uniform Buffer<br/>Host-visible]
        Staging[Staging Buffer<br/>Host-visible]
    end

    subgraph "Image Types"
        Texture[Texture<br/>Sampled]
        RT[Render Target<br/>Color Attachment]
        Depth[Depth Buffer<br/>Depth Attachment]
    end

    VMA --> Vertex
    VMA --> Index
    VMA --> Uniform
    VMA --> Staging
    VMA --> Texture
    VMA --> RT
    VMA --> Depth
```

### Pipeline Module

Graphics and compute pipeline configuration.

```mermaid
flowchart LR
    subgraph "Inputs"
        Shaders[Shader Modules]
        Layout[Pipeline Layout]
        VertexInput[Vertex Format]
    end

    subgraph "Builder"
        PB[GraphicsPipelineBuilder]
    end

    subgraph "Configuration"
        Topology[Primitive Topology]
        Rasterization[Rasterization State]
        DepthStencil[Depth/Stencil]
        Blending[Color Blending]
        Dynamic[Dynamic States]
    end

    subgraph "Output"
        Pipeline[Graphics Pipeline]
    end

    Shaders --> PB
    Layout --> PB
    VertexInput --> PB
    PB --> Topology
    PB --> Rasterization
    PB --> DepthStencil
    PB --> Blending
    PB --> Dynamic
    Topology --> Pipeline
    Rasterization --> Pipeline
    DepthStencil --> Pipeline
    Blending --> Pipeline
    Dynamic --> Pipeline
```

### RenderPass Module

High-level rendering abstractions using Vulkan 1.3 dynamic rendering.

```mermaid
classDiagram
    class Subpass~SlotEnum~ {
        #get_or_create_image()
        #transfer()
    }

    class ScreenSpacePass~SlotEnum~ {
        #render_fullscreen()
        #create_default_sampler()
    }

    class ZPass {
        +execute(cmd, scene)
    }

    class ColorPass {
        +execute(cmd, scene)
    }

    class ToneMappingPass {
        +execute(cmd, hdrInput)
    }

    Subpass <|-- ScreenSpacePass
    Subpass <|-- ZPass
    Subpass <|-- ColorPass
    ScreenSpacePass <|-- ToneMappingPass
```

## Typical Application Flow

```mermaid
flowchart TB
    subgraph "Initialization"
        I1[Create Instance]
        I2[Create Device]
        I3[Create Allocator]
        I4[Create Resources]
    end

    subgraph "Main Loop"
        M1[Acquire Image]
        M2[Record Commands]
        M3[Submit]
        M4[Present]
    end

    subgraph "Cleanup"
        C1[Destroy Resources]
        C2[Destroy Allocator]
        C3[Destroy Device]
    end

    I1 --> I2 --> I3 --> I4
    I4 --> M1
    M1 --> M2 --> M3 --> M4
    M4 --> M1
    M4 -.-> |Exit| C1
    C1 --> C2 --> C3
```

## Error Handling Strategy

```mermaid
flowchart TD
    subgraph "Vulkan Call"
        Call[API Call]
        Result{Result?}
    end

    subgraph "Success Path"
        Continue[Continue Execution]
    end

    subgraph "Error Path"
        Throw[Throw Exception]
        Unwind[Stack Unwinding]
        RAII[RAII Cleanup]
    end

    Call --> Result
    Result -->|VK_SUCCESS| Continue
    Result -->|VK_ERROR_*| Throw
    Throw --> Unwind
    Unwind --> RAII
```

All Vulkan errors are converted to exceptions with source location information, enabling precise error reporting while RAII ensures proper cleanup.

## Next Steps

- [Memory Management](./memory-management) - Deep dive into allocation strategies
- [Resource Tracking](./resource-tracking) - Automatic barrier generation
- [Render Passes](./render-passes) - High-level rendering patterns
