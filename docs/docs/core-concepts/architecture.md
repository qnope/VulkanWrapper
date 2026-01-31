---
sidebar_position: 1
---

# Architecture

VulkanWrapper is designed around several key architectural principles that make Vulkan development safer and more intuitive.

## Layered Design

The library is organized into logical layers, each building on the previous:

```
┌─────────────────────────────────────────────────┐
│              Application Layer                   │
│         (Your code, examples, games)            │
├─────────────────────────────────────────────────┤
│             High-Level Abstractions             │
│   (RenderPass, Model, Material, RayTracing)     │
├─────────────────────────────────────────────────┤
│              Core Abstractions                   │
│  (Pipeline, Descriptors, Image, Memory, Sync)   │
├─────────────────────────────────────────────────┤
│              Vulkan Foundation                   │
│      (Instance, Device, Queue, Swapchain)       │
├─────────────────────────────────────────────────┤
│              Vulkan-Hpp                          │
│         (C++ bindings for Vulkan)               │
└─────────────────────────────────────────────────┘
```

## Module Organization

### Vulkan Foundation (`vw::`)

The base layer provides core Vulkan object wrappers:

- **Instance**: Vulkan instance with validation layer support
- **Device**: Logical device with queue management
- **PhysicalDevice**: GPU enumeration and feature queries
- **Swapchain**: Presentation chain management

### Memory Management (`vw::`)

Resource allocation built on VMA (Vulkan Memory Allocator):

- **Allocator**: Centralized memory allocation
- **Buffer**: Type-safe buffer templates
- **StagingBufferManager**: CPU-to-GPU transfer batching
- **UniformBufferAllocator**: Aligned uniform allocation

### Image Handling (`vw::`)

Complete texture management:

- **Image**: 2D/3D images with format handling
- **ImageView**: Subresource views
- **Sampler**: Filtering and addressing
- **CombinedImage**: Convenience wrapper for descriptor binding

### Pipeline System (`vw::`)

Graphics and compute pipeline management:

- **Pipeline**: Pipeline with builder pattern
- **PipelineLayout**: Descriptor and push constant layouts
- **ShaderModule**: SPIR-V shader loading

### Descriptor System (`vw::`)

Resource binding infrastructure:

- **DescriptorSetLayout**: Layout definitions
- **DescriptorPool/Set**: Allocation and management
- **DescriptorAllocator**: High-level allocation tracking

### Synchronization (`vw::`)

Barrier and sync primitive management:

- **Fence/Semaphore**: Sync primitives
- **ResourceTracker**: Automatic barrier generation

### Render Pass (`vw::`)

Modern rendering abstractions (Vulkan 1.3 dynamic rendering):

- **Subpass**: Base class with lazy image caching
- **ScreenSpacePass**: Full-screen post-processing
- **SkyPass/SunLightPass/SkyLightPass**: Physical sky system
- **ToneMappingPass**: HDR to LDR conversion

### Ray Tracing (`vw::rt`)

Hardware ray tracing support:

- **BottomLevelAccelerationStructure**: Geometry BLAS
- **TopLevelAccelerationStructure**: Instance TLAS
- **RayTracingPipeline**: RT pipeline management
- **ShaderBindingTable**: SBT setup

### Model System (`vw::Model`)

3D asset handling:

- **Mesh**: Geometry data
- **Scene**: Container for mesh instances
- **MeshManager**: Loading via Assimp

### Material System (`vw::Model::Material`)

Extensible material handling:

- **Material**: Material reference with type tag
- **BindlessMaterialManager**: Handler-based material system
- **IMaterialTypeHandler**: Material type interface

## Namespace Conventions

VulkanWrapper uses a flat namespace structure with sub-namespaces for specific domains:

| Namespace | Purpose |
|-----------|---------|
| `vw` | Main namespace for all core types |
| `vw::Barrier` | Barrier-related utilities |
| `vw::Model` | 3D model and mesh types |
| `vw::Model::Material` | Material system types |
| `vw::rt` | Ray tracing types |

## Dependency Flow

Dependencies flow downward - higher-level modules depend on lower-level ones:

```
RenderPass ──────► Pipeline ──────► Shader
     │                │
     ▼                ▼
  Image ◄──────── Descriptors
     │                │
     ▼                ▼
  Memory ◄──────── Device
     │                │
     └────────► Instance
```

This design ensures:
- No circular dependencies
- Clear initialization order
- Predictable destruction order (RAII)
