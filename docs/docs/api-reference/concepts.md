---
sidebar_position: 3
---

# Concepts & Type Constraints

VulkanWrapper uses C++23 concepts and `requires` clauses instead of SFINAE. This page documents all concepts, template constraints, and strong types.

## Vertex Concept

**Header:** `Descriptors/Vertex.h`

The `Vertex` concept constrains types that can be used as vertex input data in graphics pipelines.

### Definition

```cpp
template <typename T>
concept Vertex =
    std::is_standard_layout_v<T> &&
    std::is_trivially_copyable_v<T> &&
    requires(T x) {
        { T::binding_description(0) }
            -> std::same_as<vk::VertexInputBindingDescription>;
        { T::attribute_descriptions(0, 0)[0] }
            -> std::convertible_to<vk::VertexInputAttributeDescription>;
    };
```

### Requirements

| Requirement | What It Checks |
|-------------|----------------|
| `std::is_standard_layout_v<T>` | Type has a well-defined memory layout for GPU upload |
| `std::is_trivially_copyable_v<T>` | Type can be safely `memcpy`'d into GPU buffers |
| `T::binding_description(int)` | Static method returning a `vk::VertexInputBindingDescription` |
| `T::attribute_descriptions(int, int)[0]` | Static method returning an indexable collection of `vk::VertexInputAttributeDescription` |

### Writing a Custom Vertex

The easiest way is to inherit from `VertexInterface<Ts...>`, which auto-generates both static methods from the template parameter types:

```cpp
struct MyVertex : vw::VertexInterface<glm::vec3, glm::vec3, glm::vec2> {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 uv{};
};

static_assert(vw::Vertex<MyVertex>);
```

`VertexInterface` uses `format_from<T>` to map C++ types to Vulkan formats:

| C++ Type | Vulkan Format |
|----------|---------------|
| `float` | `eR32Sfloat` |
| `glm::vec2` | `eR32G32Sfloat` |
| `glm::vec3` | `eR32G32B32Sfloat` |
| `glm::vec4` | `eR32G32B32A32Sfloat` |

You can also write the static methods manually:

```cpp
struct ManualVertex {
    glm::vec3 position;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription
    binding_description(int binding) {
        return {static_cast<uint32_t>(binding),
                sizeof(ManualVertex),
                vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 2>
    attribute_descriptions(int binding, int location) {
        return {{
            {static_cast<uint32_t>(location), static_cast<uint32_t>(binding),
             vk::Format::eR32G32B32Sfloat, offsetof(ManualVertex, position)},
            {static_cast<uint32_t>(location + 1), static_cast<uint32_t>(binding),
             vk::Format::eR32G32Sfloat, offsetof(ManualVertex, texCoord)}
        }};
    }
};
```

### Built-in Vertex Types

| Type | Members | Inherits From |
|------|---------|---------------|
| `Vertex3D` | `vec3 position` | `VertexInterface<vec3>` |
| `ColoredVertex<Position>` | `Position position`, `vec3 color` | `VertexInterface<Position, vec3>` |
| `ColoredAndTexturedVertex<Position>` | `Position position`, `vec3 color`, `vec2 texCoord` | `VertexInterface<Position, vec3, vec2>` |
| `FullVertex3D` | `vec3 position`, `vec3 normal`, `vec3 tangeant`, `vec3 bitangeant`, `vec2 uv` | `VertexInterface<vec3, vec3, vec3, vec3, vec2>` |

**Type aliases:**

```cpp
using ColoredVertex2D              = ColoredVertex<glm::vec2>;
using ColoredVertex3D              = ColoredVertex<glm::vec3>;
using ColoredAndTexturedVertex2D   = ColoredAndTexturedVertex<glm::vec2>;
using ColoredAndTexturedVertex3D   = ColoredAndTexturedVertex<glm::vec3>;
```

### Where the Concept Is Used

```cpp
// GraphicsPipelineBuilder -- compile-time vertex format registration
template <Vertex V>
GraphicsPipelineBuilder& add_vertex_binding();

// Example
builder.add_vertex_binding<FullVertex3D>();    // OK
builder.add_vertex_binding<int>();             // Compile error: int does not satisfy Vertex

// allocate_vertex_buffer -- constrained by Buffer<T,...> which requires trivially copyable T
auto vbuf = allocate_vertex_buffer<FullVertex3D, false>(allocator, 1000);
```

---

## Buffer Template Constraints

**Header:** `Memory/Buffer.h`

### Template Parameters

```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer : public BufferBase;
```

| Parameter | Meaning |
|-----------|---------|
| `T` | Element type stored in the buffer |
| `HostVisible` | `true` = CPU-mappable (for uploads/readback); `false` = device-local only |
| `Flags` | Vulkan buffer usage flags (use the predefined constants) |

### Host-Visible Constraint (`requires`)

Methods that access CPU-mapped memory are only available when `HostVisible` is `true`:

```cpp
// Only exists when HostVisible == true
void write(std::span<const T> data, std::size_t offset)
    requires(HostVisible);

void write(const T& element, std::size_t offset)
    requires(HostVisible);

std::vector<T> read_as_vector(std::size_t offset, std::size_t count) const
    requires(HostVisible);
```

```cpp
auto gpu_buf = create_buffer<float, false, StorageBufferUsage>(allocator, 100);
// gpu_buf.write(...);       // Compile error: HostVisible is false

auto cpu_buf = create_buffer<float, true, StagingBufferUsage>(allocator, 100);
cpu_buf.write(data, 0);      // OK: HostVisible is true
```

### Compile-Time Usage Validation

```cpp
static consteval bool does_support(vk::BufferUsageFlags usage);
```

Check at compile time whether a buffer's usage flags include a specific capability:

```cpp
using VBuf = Buffer<FullVertex3D, false, VertexBufferUsage>;

static_assert(VBuf::does_support(vk::BufferUsageFlagBits::eVertexBuffer));     // OK
static_assert(VBuf::does_support(vk::BufferUsageFlagBits::eTransferDst));      // OK
static_assert(!VBuf::does_support(vk::BufferUsageFlagBits::eUniformBuffer));   // Not included
```

### Buffer Usage Constants

Defined in `Memory/BufferUsage.h`. Each constant combines the primary usage with `eTransferDst` and `eShaderDeviceAddress`:

| Constant | Primary Flag | Also Includes |
|----------|-------------|---------------|
| `VertexBufferUsage` | `eVertexBuffer` | `eTransferDst`, `eShaderDeviceAddress`, `eAccelerationStructureBuildInputReadOnlyKHR` |
| `IndexBufferUsage` | `eIndexBuffer` | `eTransferDst`, `eShaderDeviceAddress`, `eAccelerationStructureBuildInputReadOnlyKHR` |
| `UniformBufferUsage` | `eUniformBuffer` | `eTransferDst`, `eShaderDeviceAddress` |
| `StorageBufferUsage` | `eStorageBuffer` | `eTransferDst`, `eShaderDeviceAddress` |
| `StagingBufferUsage` | `eTransferSrc` | `eTransferDst`, `eShaderDeviceAddress` |

### Buffer Factory Functions

**Header:** `Memory/AllocateBufferUtils.h`

```cpp
// Create a buffer with explicit template parameters
template <typename T, bool HostVisible, VkBufferUsageFlags2 Usage>
Buffer<T, HostVisible, Usage> create_buffer(const Allocator& allocator,
                                             vk::DeviceSize number_elements);

// Create a buffer from a Buffer type alias
template <typename BufferType>
BufferType create_buffer(const Allocator& allocator,
                         vk::DeviceSize number_elements);

// Shortcut for vertex buffers
template <typename T, bool HostVisible>
Buffer<T, HostVisible, VertexBufferUsage>
allocate_vertex_buffer(const Allocator& allocator, vk::DeviceSize size);
```

### Common Buffer Type Aliases

```cpp
// Defined in fwd.h
using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;

// Defined in Model/Mesh.h
using Vertex3DBuffer     = Buffer<Vertex3D, false, VertexBufferUsage>;
using FullVertex3DBuffer = Buffer<FullVertex3D, false, VertexBufferUsage>;

// Defined in RayTracing/GeometryReference.h
using GeometryReferenceBuffer = Buffer<GeometryReference, true, StorageBufferUsage>;
```

---

## Strong Types

**Defined in:** the `vw` namespace (exported via `vulkan3rd` module / `3rd_party.h`)

Strong types wrap `uint32_t` to prevent argument-order mistakes at compile time.

| Type | Purpose | Example |
|------|---------|---------|
| `Width` | Image or viewport width | `Width{1920}` |
| `Height` | Image or viewport height | `Height{1080}` |
| `Depth` | Image depth (3D textures) | `Depth{1}` |
| `MipLevel` | Mipmap level index | `MipLevel{0}` |
| `ArrayLayer` | Image array layer index | `ArrayLayer{0}` |

### How They Prevent Bugs

```cpp
// Without strong types -- easy to swap width and height:
allocator->create_image_2D(1080, 1920, ...);  // Bug! Height passed as width

// With strong types -- compiler catches the mistake:
allocator->create_image_2D(Width{1920}, Height{1080}, ...);  // Correct
allocator->create_image_2D(Height{1080}, Width{1920}, ...);  // Compile error!
```

### Where They Are Used

- `Allocator::create_image_2D(Width, Height, ...)`
- `RenderPass::execute(cmd, tracker, Width, Height, frame_index)`
- `RenderPipeline::execute(cmd, tracker, Width, Height, frame_index)`
- `RenderPass::get_or_create_image(slot, Width, Height, ...)`

---

## API Version Enum

```cpp
enum class ApiVersion { e10, e11, e12, e13 };
```

Used with `InstanceBuilder::setApiVersion()`:

```cpp
auto instance = InstanceBuilder()
    .setApiVersion(ApiVersion::e13)
    .setDebug()
    .build();
```

---

## RenderPipeline Constraints

`RenderPipeline::add()` uses `std::derived_from` to ensure only `RenderPass` subclasses can be added:

```cpp
template <std::derived_from<RenderPass> T>
T& add(std::unique_ptr<T> pass);
```

```cpp
pipeline.add(std::make_unique<ZPass>(...));             // OK
pipeline.add(std::make_unique<ToneMappingPass>(...));   // OK
pipeline.add(std::make_unique<std::string>("hello"));   // Compile error
```

---

## ObjectWithHandle Pattern

**Header:** `Utils/ObjectWithHandle.h`

The `ObjectWithHandle<T>` template provides a uniform `.handle()` accessor. The template parameter `T` determines ownership:

| `T` | Ownership | `handle()` Returns |
|-----|-----------|-------------------|
| `vk::Pipeline` | Non-owning (caller manages lifetime) | `vk::Pipeline` |
| `vk::UniquePipeline` | Owning (RAII, auto-destroyed) | `vk::Pipeline` |

`ObjectWithUniqueHandle<T>` is a type alias, not a separate class:

```cpp
template <typename UniqueHandle>
using ObjectWithUniqueHandle = ObjectWithHandle<UniqueHandle>;
```

### Range Adaptor: `to_handle`

Extracts raw Vulkan handles from collections of wrapper objects:

```cpp
std::vector<Pipeline> pipelines = ...;

// Extract raw vk::Pipeline handles
auto handles = pipelines | vw::to_handle | std::ranges::to<std::vector>();
```

Works with both value semantics (`x.handle()`) and pointer semantics (`x->handle()`).
