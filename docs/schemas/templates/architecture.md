# Architecture Diagram Templates

## Template 1: Module Overview

Use for showing relationships between library modules.

```mermaid
graph TB
    subgraph "Application Layer"
        App[Your Application]
    end

    subgraph "VulkanWrapper Modules"
        direction TB
        Vulkan[Vulkan<br/>Instance, Device, Queue]
        Memory[Memory<br/>Allocator, Buffers]
        Image[Image<br/>Images, Views, Samplers]
        Pipeline[Pipeline<br/>Shaders, Graphics]
        Descriptors[Descriptors<br/>Sets, Layouts, Pools]
        RenderPass[RenderPass<br/>Subpass, ScreenSpace]
        RayTracing[RayTracing<br/>BLAS, TLAS, Pipeline]
        Sync[Synchronization<br/>Barriers, Fences]
    end

    subgraph "Vulkan API"
        VkAPI[Vulkan Driver]
    end

    App --> Vulkan
    App --> Memory
    App --> Image
    App --> Pipeline
    App --> Descriptors
    App --> RenderPass
    App --> RayTracing
    App --> Sync

    Vulkan --> VkAPI
    Memory --> VkAPI
    Image --> VkAPI
    Pipeline --> VkAPI
    Descriptors --> VkAPI
    RenderPass --> VkAPI
    RayTracing --> VkAPI
    Sync --> VkAPI
```

## Template 2: Component Dependencies

Use for showing how modules depend on each other.

```mermaid
graph LR
    subgraph "Core"
        Instance
        Device
        Queue
    end

    subgraph "Memory"
        Allocator
        Buffer
        Image
    end

    subgraph "Graphics"
        Pipeline
        DescriptorSet
        RenderPass
    end

    Instance --> Device
    Device --> Queue
    Device --> Allocator
    Allocator --> Buffer
    Allocator --> Image
    Device --> Pipeline
    Device --> DescriptorSet
    Pipeline --> RenderPass
    DescriptorSet --> RenderPass
```

## Template 3: Layered Architecture

Use for showing abstraction layers.

```mermaid
graph TB
    subgraph "High-Level API"
        ScreenSpacePass
        DeferredRenderer
        RayTracedScene
    end

    subgraph "Mid-Level Abstractions"
        Pipeline
        DescriptorSet
        CommandPool
        Subpass
    end

    subgraph "Low-Level Wrappers"
        Buffer
        Image
        ShaderModule
        Sampler
    end

    subgraph "Vulkan Core"
        Device
        Allocator
        Instance
    end

    ScreenSpacePass --> Pipeline
    ScreenSpacePass --> Subpass
    DeferredRenderer --> ScreenSpacePass
    RayTracedScene --> Pipeline

    Pipeline --> ShaderModule
    DescriptorSet --> Sampler
    Subpass --> Image
    CommandPool --> Device

    Buffer --> Allocator
    Image --> Allocator
    ShaderModule --> Device
    Sampler --> Device

    Allocator --> Device
    Device --> Instance
```

## Template 4: Initialization Flow

Use for showing startup sequence.

```mermaid
flowchart TD
    Start([Start]) --> Instance
    Instance[Create Instance] --> PhysicalDevice
    PhysicalDevice[Select Physical Device] --> Device
    Device[Create Logical Device] --> Queues
    Queues[Get Queues] --> Allocator
    Allocator[Create Allocator] --> Ready
    Ready([Ready to Render])

    Instance -.-> |Validation Layers| Debug[Debug Utils]
    PhysicalDevice -.-> |Requirements| Features[Check Features]
```

## Customization Guide

1. **Replace placeholders** with actual component names
2. **Add/remove nodes** as needed for your documentation
3. **Use subgraphs** to group related components
4. **Add edge labels** for relationship descriptions
5. **Use shapes** to distinguish component types:
   - `[Rectangle]` for classes/modules
   - `([Stadium])` for entry/exit points
   - `{Diamond}` for decisions
   - `[(Database)]` for storage
