# Class Hierarchy Diagram Templates

## Template 1: Handle Wrapper Hierarchy

Use for documenting the Vulkan handle wrapper pattern.

```mermaid
classDiagram
    class ObjectWithHandle~T~ {
        #T m_handle
        +handle() T
        +operator bool()
    }

    class ObjectWithUniqueHandle~T~ {
        #vk::UniqueHandle~T~ m_handle
        +handle() T
        +release() T
    }

    class Device {
        -vk::UniqueDevice m_device
        +handle() vk::Device
        +wait_idle()
        +find_queue() Queue&
    }

    class Image {
        -VmaAllocation m_allocation
        +handle() vk::Image
        +format() vk::Format
        +extent() vk::Extent3D
    }

    class Pipeline {
        +handle() vk::Pipeline
        +layout() PipelineLayout&
    }

    ObjectWithHandle <|-- ObjectWithUniqueHandle
    ObjectWithHandle <|-- Image
    ObjectWithUniqueHandle <|-- Device
    ObjectWithUniqueHandle <|-- Pipeline
```

## Template 2: Buffer Type Hierarchy

Use for documenting type-safe buffers.

```mermaid
classDiagram
    class Buffer~T, HostVisible, Usage~ {
        +size() size_t
        +byte_size() size_t
        +handle() vk::Buffer
        +data() span~T~ [HostVisible=true]
    }

    class VertexBuffer~T~ {
        <<typedef>>
        Buffer~T, false, VertexBufferUsage~
    }

    class IndexBuffer {
        <<typedef>>
        Buffer~uint32_t, false, IndexBufferUsage~
    }

    class UniformBuffer~T~ {
        <<typedef>>
        Buffer~T, true, UniformBufferUsage~
    }

    class StagingBuffer~T~ {
        <<typedef>>
        Buffer~T, true, TransferSrcUsage~
    }

    Buffer <|-- VertexBuffer
    Buffer <|-- IndexBuffer
    Buffer <|-- UniformBuffer
    Buffer <|-- StagingBuffer

    note for Buffer "Template parameters:\nT = Element type\nHostVisible = CPU accessible\nUsage = VkBufferUsageFlags"
```

## Template 3: Render Pass Hierarchy

Use for documenting the render pass abstraction.

```mermaid
classDiagram
    class Subpass~SlotEnum~ {
        #shared_ptr~Device~ m_device
        #shared_ptr~Allocator~ m_allocator
        #get_or_create_image() CachedImage&
        #transfer() Transfer&
    }

    class ScreenSpacePass~SlotEnum~ {
        #shared_ptr~Sampler~ m_sampler
        #create_default_sampler() Sampler
        #render_fullscreen()
    }

    class ZPass {
        +execute()
    }

    class ColorPass {
        +execute()
    }

    class AmbientOcclusionPass {
        +execute()
        -m_sampleIndex uint32_t
    }

    class ToneMappingPass {
        +execute()
    }

    Subpass <|-- ScreenSpacePass
    Subpass <|-- ZPass
    Subpass <|-- ColorPass
    ScreenSpacePass <|-- AmbientOcclusionPass
    ScreenSpacePass <|-- ToneMappingPass
```

## Template 4: Exception Hierarchy

Use for documenting error handling.

```mermaid
classDiagram
    class Exception {
        -string m_message
        -source_location m_location
        +what() const char*
        +location() source_location
    }

    class VulkanException {
        -vk::Result m_result
        +result() vk::Result
    }

    class SDLException {
        +sdl_error() string
    }

    class VMAException {
        -VkResult m_result
    }

    class FileException {
        -string m_path
        +path() string
    }

    class ShaderCompilationException {
        -string m_errors
        +errors() string
    }

    class LogicException {
        <<abstract>>
    }

    class OutOfRangeException
    class InvalidStateException
    class NullPointerException

    Exception <|-- VulkanException
    Exception <|-- SDLException
    Exception <|-- VMAException
    Exception <|-- FileException
    Exception <|-- ShaderCompilationException
    Exception <|-- LogicException
    LogicException <|-- OutOfRangeException
    LogicException <|-- InvalidStateException
    LogicException <|-- NullPointerException
```

## Template 5: Material System Hierarchy

Use for documenting polymorphic systems.

```mermaid
classDiagram
    class Material {
        <<abstract>>
        +bind(cmd, layout)
        +type_index() type_index
    }

    class ColoredMaterial {
        -vec4 m_color
        +color() vec4
    }

    class TexturedMaterial {
        -CombinedImage m_texture
        +texture() CombinedImage&
    }

    class PBRMaterial {
        -CombinedImage m_albedo
        -CombinedImage m_normal
        -CombinedImage m_metalRoughness
        +albedo() CombinedImage&
        +normal() CombinedImage&
    }

    Material <|-- ColoredMaterial
    Material <|-- TexturedMaterial
    Material <|-- PBRMaterial
```

## Template 6: Builder Pattern Classes

Use for documenting builder APIs.

```mermaid
classDiagram
    class GraphicsPipelineBuilder {
        -shared_ptr~Device~ m_device
        -vector~ShaderStageInfo~ m_shaders
        -VertexInputState m_vertexInput
        +set_layout(PipelineLayout) Self&
        +add_shader(stage, module) Self&
        +set_vertex_input~V~() Self&
        +set_topology(topology) Self&
        +build() shared_ptr~Pipeline~
    }

    class InstanceBuilder {
        -bool m_debug
        -ApiVersion m_version
        +setDebug() Self&
        +addPortability() Self&
        +setApiVersion(ver) Self&
        +build() shared_ptr~Instance~
    }

    class DeviceFinder {
        -Instance& m_instance
        -vector~QueueRequest~ m_queues
        +with_queue(flags) Self&
        +with_synchronization_2() Self&
        +with_dynamic_rendering() Self&
        +build() shared_ptr~Device~
    }

    note for GraphicsPipelineBuilder "Fluent API:\nbuilder.option1().option2().build()"
```

## Customization Guide

1. **Use `~T~` notation** for template parameters
2. **Mark abstract classes** with `<<abstract>>`
3. **Mark typedefs** with `<<typedef>>`
4. **Show key methods** only, not all
5. **Add notes** for important implementation details
6. **Use inheritance arrows** pointing from derived to base
