# VulkanWrapper Architecture Improvement Proposals

This document proposes improvements to enhance safety, usability, and consistency of the VulkanWrapper API.

## Executive Summary

The VulkanWrapper library has an excellent foundation with strong patterns:
- **Builder pattern** consistently applied throughout
- **RAII** with `vk::UniqueHandle` wrappers
- **Type safety** via template-based buffers with compile-time validation
- **Automatic barrier generation** via `ResourceTracker` with interval-based tracking

The proposed improvements fall into three tiers:
1. **Safety Critical** - Issues that can cause runtime failures or undefined behavior
2. **API Usability** - Improvements to make the API more intuitive and harder to misuse
3. **Architecture Enhancements** - Features that increase flexibility without breaking existing code

---

## Tier 1: Safety Critical Improvements

### 1.1 Replace `assert()` with `LogicException`

**Problem**: Several locations use `assert()` which disappears in Release builds, leading to undefined behavior.

**Affected Files**:
- `VulkanWrapper/src/VulkanWrapper/Vulkan/DeviceFinder.cpp:43`
- `VulkanWrapper/src/VulkanWrapper/Vulkan/Device.cpp:31`

**Current Code** (DeviceFinder.cpp):
```cpp
for (auto &information : m_physicalDevicesInformation) {
    auto index = index_if(information.queuesInformation, queueFamilyHandled);
    assert(index);  // Silent failure in Release!
    ++information.queuesInformation[*index].numberAsked;
}
```

**Proposed Fix**:
```cpp
for (auto &information : m_physicalDevicesInformation) {
    auto index = index_if(information.queuesInformation, queueFamilyHandled);
    if (!index) {
        throw LogicException::invalid_state(
            "Queue family supporting requested flags not found after filtering");
    }
    ++information.queuesInformation[*index].numberAsked;
}
```

**Current Code** (Device.cpp):
```cpp
const PresentQueue &Device::presentQueue() const {
    assert(m_impl->presentQueue);
    return m_impl->presentQueue.value();
}
```

**Proposed Fix** (Option A - throw on missing):
```cpp
const PresentQueue &Device::presentQueue() const {
    if (!m_impl->presentQueue) {
        throw LogicException::invalid_state(
            "presentQueue() called but device was not created with presentation support. "
            "Use with_presentation() in DeviceFinder.");
    }
    return m_impl->presentQueue.value();
}
```

**Proposed Fix** (Option B - explicit API):
```cpp
[[nodiscard]] bool Device::has_present_queue() const noexcept {
    return m_impl->presentQueue.has_value();
}

[[nodiscard]] std::optional<std::reference_wrapper<const PresentQueue>>
Device::presentQueue() const noexcept {
    if (m_impl->presentQueue) {
        return std::cref(m_impl->presentQueue.value());
    }
    return std::nullopt;
}
```

---

### 1.2 Fix DescriptorAllocator Hash Function

**Problem**: The hash function always returns 0, causing O(n) lookup in hash maps.

**File**: `VulkanWrapper/include/VulkanWrapper/Descriptors/DescriptorAllocator.h:77-82`

**Current Code**:
```cpp
template <> struct std::hash<vw::DescriptorAllocator> {
    std::size_t operator()(const vw::DescriptorAllocator &allocator) const noexcept {
        return 0;  // All allocators hash to same bucket!
    }
};
```

**Proposed Fix**:
```cpp
template <> struct std::hash<vw::DescriptorAllocator> {
    std::size_t operator()(const vw::DescriptorAllocator &allocator) const noexcept {
        std::size_t seed = 0;

        // Hash buffer updates
        for (const auto &buf : allocator.buffer_updates()) {
            seed ^= std::hash<VkBuffer>{}(buf.info.buffer) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<VkDeviceSize>{}(buf.info.offset) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        // Hash image updates
        for (const auto &img : allocator.image_updates()) {
            seed ^= std::hash<VkImage>{}(img.image) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        return seed;
    }
};
```

**Note**: This requires adding const accessor methods to `DescriptorAllocator`:
```cpp
[[nodiscard]] const std::vector<BufferUpdate>& buffer_updates() const noexcept { return m_bufferUpdate; }
[[nodiscard]] const std::vector<ImageUpdate>& image_updates() const noexcept { return m_imageUpdate; }
```

---

### 1.3 Add Builder Validation

**Problem**: Builders allow creating invalid objects (e.g., pipeline without shaders).

**File**: `VulkanWrapper/src/VulkanWrapper/Pipeline/Pipeline.cpp:102`

**Proposed Enhancement**:
```cpp
std::shared_ptr<const Pipeline> GraphicsPipelineBuilder::build() && {
    // Validate required state
    if (m_shaderModules.empty()) {
        throw LogicException::invalid_state(
            "GraphicsPipelineBuilder: At least one shader module is required");
    }

    if (!m_shaderModules.contains(vk::ShaderStageFlagBits::eVertex)) {
        throw LogicException::invalid_state(
            "GraphicsPipelineBuilder: Vertex shader is required for graphics pipeline");
    }

    if (m_colorAttachmentFormats.empty() && m_depthFormat == vk::Format::eUndefined) {
        throw LogicException::invalid_state(
            "GraphicsPipelineBuilder: At least one color or depth attachment is required");
    }

    // Warn about potentially unintended states
    if (m_viewport && !m_scissor) {
        // Consider logging a warning
    }

    // Existing build logic...
}
```

**Similar validation should be added to**:
- `DescriptorSetLayoutBuilder::build()` - validate at least one binding
- `PipelineLayoutBuilder::build()` - validate descriptor set layouts exist
- `RayTracingPipelineBuilder::build()` - validate ray generation shader exists

---

### 1.4 Fix UniformBufferChunk Type Safety

**Problem**: `UniformBufferChunk::copy()` uses raw cast from `shared_ptr` to raw pointer.

**File**: `VulkanWrapper/include/VulkanWrapper/Memory/UniformBufferAllocator.h:32-38`

**Current Code**:
```cpp
void copy(const T& value) {
    if (!buffer_ref) {
        throw std::runtime_error("Buffer reference is null");
    }
    BufferBase* base_ptr = static_cast<BufferBase*>(buffer_ref.get());
    base_ptr->generic_copy(&value, sizeof(T), offset);
}
```

**Issues**:
1. Uses `std::runtime_error` instead of `LogicException`
2. Unnecessary cast (Buffer inherits from BufferBase)
3. Could use `buffer_ref` directly

**Proposed Fix**:
```cpp
void copy(const T& value) {
    if (!buffer_ref) {
        throw LogicException::null_pointer("UniformBufferChunk::buffer_ref");
    }
    buffer_ref->generic_copy(&value, sizeof(T), offset);
}

void copy(std::span<const T> data) {
    if (!buffer_ref) {
        throw LogicException::null_pointer("UniformBufferChunk::buffer_ref");
    }
    buffer_ref->generic_copy(data.data(), data.size_bytes(), offset);
}
```

---

## Tier 2: API Usability Improvements

### 2.1 Clarify ObjectWithHandle Behavior

**Problem**: The `handle()` method's behavior depends on `sizeof(T)`, which is implicit and surprising.

**File**: `VulkanWrapper/include/VulkanWrapper/Utils/ObjectWithHandle.h`

**Current Code**:
```cpp
auto handle() const noexcept {
    if constexpr (sizeof(T) > sizeof(void *))
        return *m_handle;  // Dereference for UniqueHandle
    else
        return m_handle;   // Return directly for raw handles
}
```

**Proposed Enhancement** - Use explicit type traits:
```cpp
namespace detail {
    template <typename T>
    concept VulkanUniqueHandle = requires(T t) {
        { t.get() } -> std::convertible_to<typename T::element_type>;
    };
}

template <typename T>
class ObjectWithHandle {
public:
    using handle_type = std::conditional_t<
        detail::VulkanUniqueHandle<T>,
        typename T::element_type,
        T
    >;

    ObjectWithHandle(T handle) noexcept
        : m_handle{std::move(handle)} {}

    [[nodiscard]] handle_type handle() const noexcept {
        if constexpr (detail::VulkanUniqueHandle<T>) {
            return m_handle.get();
        } else {
            return m_handle;
        }
    }

private:
    T m_handle;
};
```

---

### 2.2 Explicit Descriptor Binding Numbers

**Problem**: Auto-increment binding numbers can lead to silent mismatches with shaders.

**File**: `VulkanWrapper/src/VulkanWrapper/Descriptors/DescriptorSetLayout.cpp`

**Current API**:
```cpp
auto layout = DescriptorSetLayoutBuilder(device)
    .with_uniform_buffer(vk::ShaderStageFlagBits::eVertex)  // binding 0
    .with_combined_image(vk::ShaderStageFlagBits::eFragment) // binding 1
    .build();
```

**Proposed Enhancement** - Add explicit binding overloads:
```cpp
class DescriptorSetLayoutBuilder {
public:
    // Existing auto-increment API (keep for convenience)
    DescriptorSetLayoutBuilder&& with_uniform_buffer(
        vk::ShaderStageFlags stages, int count = 1) &&;

    // New explicit binding API
    DescriptorSetLayoutBuilder&& with_uniform_buffer_at(
        uint32_t binding, vk::ShaderStageFlags stages, int count = 1) &&;

    DescriptorSetLayoutBuilder&& with_combined_image_at(
        uint32_t binding, vk::ShaderStageFlags stages, int count = 1) &&;

    // Validation in build()
    std::shared_ptr<DescriptorSetLayout> build() && {
        // Check for binding conflicts
        std::set<uint32_t> used_bindings;
        for (const auto& binding : m_bindings) {
            if (!used_bindings.insert(binding.binding).second) {
                throw LogicException::invalid_state(
                    std::format("Duplicate descriptor binding: {}", binding.binding));
            }
        }
        // ... existing build logic
    }
};
```

**Example Usage**:
```cpp
// Explicit bindings - matches shader layout exactly
auto layout = DescriptorSetLayoutBuilder(device)
    .with_uniform_buffer_at(0, vk::ShaderStageFlagBits::eVertex)
    .with_combined_image_at(2, vk::ShaderStageFlagBits::eFragment)  // Skip binding 1
    .build();
```

---

### 2.3 Improve Buffer Copy Semantics

**Problem**: Multiple `copy()` methods with different semantics can be confusing.

**File**: `VulkanWrapper/include/VulkanWrapper/Memory/Buffer.h`

**Current API**:
```cpp
void copy(std::span<const T> span, std::size_t offset);      // Type-safe
void copy(const T &object, std::size_t offset);              // Type-safe
template <typename U>
void generic_copy(std::span<const U> span, std::size_t offset); // Type-unsafe
```

**Proposed Enhancement** - Clearer naming:
```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer : public BufferBase {
public:
    // Type-safe copies (preferred)
    void write(std::span<const T> data, std::size_t element_offset = 0)
        requires(HostVisible);

    void write(const T& value, std::size_t element_offset = 0)
        requires(HostVisible);

    // Read data back
    [[nodiscard]] std::vector<T> read(std::size_t element_offset, std::size_t count) const
        requires(HostVisible);

    [[nodiscard]] T read_one(std::size_t element_offset = 0) const
        requires(HostVisible);

    // Raw byte access (explicit about unsafety)
    void write_raw(const void* data, std::size_t byte_size, std::size_t byte_offset)
        requires(HostVisible);

    [[nodiscard]] std::vector<std::byte> read_raw(std::size_t byte_offset, std::size_t byte_size) const
        requires(HostVisible);

    // Deprecate old names
    [[deprecated("Use write() instead")]]
    void copy(std::span<const T> span, std::size_t offset) requires(HostVisible) {
        write(span, offset);
    }
};
```

---

### 2.4 Resource State Builder Pattern

**Problem**: Creating `Barrier::ResourceState` variants is verbose.

**Current Usage**:
```cpp
tracker.request(vw::Barrier::ImageState{
    .image = swapchainBuffer->image()->handle(),
    .subresourceRange = swapchainBuffer->subresource_range(),
    .layout = vk::ImageLayout::ePresentSrcKHR,
    .stage = vk::PipelineStageFlagBits2::eNone,
    .access = vk::AccessFlagBits2::eNone
});
```

**Proposed Enhancement** - Fluent builders:
```cpp
namespace Barrier {
    class ImageStateBuilder {
    public:
        ImageStateBuilder(vk::Image image) : m_state{.image = image} {}

        ImageStateBuilder& subresource(vk::ImageSubresourceRange range) & {
            m_state.subresourceRange = range;
            return *this;
        }

        ImageStateBuilder& layout(vk::ImageLayout layout) & {
            m_state.layout = layout;
            return *this;
        }

        ImageStateBuilder& for_color_attachment_write() & {
            m_state.stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            m_state.access = vk::AccessFlagBits2::eColorAttachmentWrite;
            return *this;
        }

        ImageStateBuilder& for_shader_read() & {
            m_state.stage = vk::PipelineStageFlagBits2::eFragmentShader;
            m_state.access = vk::AccessFlagBits2::eShaderRead;
            return *this;
        }

        ImageStateBuilder& for_present() & {
            m_state.layout = vk::ImageLayout::ePresentSrcKHR;
            m_state.stage = vk::PipelineStageFlagBits2::eNone;
            m_state.access = vk::AccessFlagBits2::eNone;
            return *this;
        }

        [[nodiscard]] ImageState build() const { return m_state; }
        operator ImageState() const { return m_state; }

    private:
        ImageState m_state{};
    };
}

// Usage becomes:
tracker.request(
    Barrier::ImageStateBuilder(swapchainBuffer->image()->handle())
        .subresource(swapchainBuffer->subresource_range())
        .for_present()
);
```

---

## Tier 3: Architecture Enhancements

### 3.1 Configurable Buffer Pool Sizes

**Problem**: `BufferList` uses hardcoded 16MB buffer size.

**File**: `VulkanWrapper/include/VulkanWrapper/Memory/BufferList.h:20`

**Current Code**:
```cpp
constexpr std::size_t buffer_size = 1 << 24;  // 16 MB hardcoded
```

**Proposed Enhancement**:
```cpp
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class BufferList {
public:
    struct Config {
        std::size_t buffer_size = 1 << 24;  // 16 MB default
        std::size_t max_buffers = 64;       // Optional limit
    };

    BufferList(std::shared_ptr<const Allocator> allocator,
               Config config = {}) noexcept
        : m_allocator{std::move(allocator)}
        , m_config{config} {}

    // ...
};

// Usage:
BufferList<Vertex, false, VertexBufferUsage> vertices(
    allocator,
    {.buffer_size = 64 * 1024 * 1024}  // 64MB for large meshes
);
```

---

### 3.2 Timeline Semaphore Support

**Problem**: Only binary semaphores are exposed; timeline semaphores enable more efficient synchronization.

**Proposed Addition**:
```cpp
class TimelineSemaphore {
public:
    TimelineSemaphore(std::shared_ptr<const Device> device, uint64_t initial_value = 0);

    [[nodiscard]] vk::Semaphore handle() const noexcept;
    [[nodiscard]] uint64_t current_value() const;

    void signal(uint64_t value);
    void wait(uint64_t value, uint64_t timeout_ns = UINT64_MAX);

    // For submit operations
    [[nodiscard]] vk::SemaphoreSubmitInfo submit_info(
        uint64_t value,
        vk::PipelineStageFlags2 stage) const;
};

class TimelineSemaphoreBuilder {
public:
    TimelineSemaphoreBuilder(std::shared_ptr<const Device> device);
    TimelineSemaphoreBuilder&& with_initial_value(uint64_t value) &&;
    TimelineSemaphore build() &&;
};
```

---

### 3.3 Scoped Resource Tracking

**Problem**: `ResourceTracker` requires manual `track()`/`request()`/`flush()` calls.

**Proposed Enhancement** - RAII scope guards:
```cpp
class ScopedResourceUsage {
public:
    ScopedResourceUsage(ResourceTracker& tracker, vk::CommandBuffer cmd)
        : m_tracker(tracker), m_cmd(cmd) {}

    ~ScopedResourceUsage() {
        m_tracker.flush(m_cmd);
    }

    // Convenience methods that track and request in one call
    void use_image_as_color_attachment(const Image& image);
    void use_image_as_shader_resource(const Image& image);
    void use_buffer_as_vertex_input(const BufferBase& buffer);
    // ...

private:
    ResourceTracker& m_tracker;
    vk::CommandBuffer m_cmd;
};

// Usage:
{
    ScopedResourceUsage usage(tracker, cmd);
    usage.use_image_as_color_attachment(*renderTarget);
    usage.use_buffer_as_vertex_input(*vertexBuffer);
    // Automatic flush on scope exit
}
```

---

### 3.4 Better Queue Family Abstraction

**Problem**: Queue family selection is implicit; users may need specific queue families.

**Proposed Enhancement**:
```cpp
enum class QueueCapability {
    Graphics,
    Compute,
    Transfer,
    SparseBinding,
    Present
};

class QueueFamily {
public:
    [[nodiscard]] uint32_t index() const noexcept;
    [[nodiscard]] bool supports(QueueCapability cap) const noexcept;
    [[nodiscard]] uint32_t queue_count() const noexcept;
};

class Device {
public:
    // New API
    [[nodiscard]] std::span<const QueueFamily> queue_families() const noexcept;
    [[nodiscard]] Queue& queue(uint32_t family_index, uint32_t queue_index);
    [[nodiscard]] std::optional<Queue&> find_queue(QueueCapability cap);

    // Keep existing convenience methods
    [[nodiscard]] Queue& graphicsQueue();
};
```

---

## Implementation Priority

| Priority | Item | Effort | Impact |
|----------|------|--------|--------|
| **P0** | 1.1 Replace assert() with exceptions | Low | High |
| **P0** | 1.3 Add builder validation | Medium | High |
| **P1** | 1.2 Fix DescriptorAllocator hash | Low | Medium |
| **P1** | 1.4 Fix UniformBufferChunk safety | Low | Medium |
| **P1** | 2.2 Explicit descriptor bindings | Medium | High |
| **P2** | 2.1 Clarify ObjectWithHandle | Low | Low |
| **P2** | 2.3 Improve buffer copy semantics | Medium | Medium |
| **P2** | 2.4 Resource state builders | Medium | Medium |
| **P3** | 3.1 Configurable buffer sizes | Low | Low |
| **P3** | 3.2 Timeline semaphores | Medium | Medium |
| **P3** | 3.3 Scoped resource tracking | Medium | Medium |
| **P3** | 3.4 Queue family abstraction | High | Medium |

---

## Breaking Changes Analysis

### Non-Breaking (Safe to implement now):
- 1.1 - Assertions → Exceptions (existing correct code unaffected)
- 1.2 - Hash function fix (internal implementation detail)
- 1.3 - Builder validation (catches errors earlier)
- 1.4 - UniformBufferChunk fix (internal implementation)
- 2.2 - Explicit binding overloads (additive)
- 3.1 - Configurable sizes (uses defaults)

### Potentially Breaking (Requires migration):
- 2.1 - ObjectWithHandle refactor (may change `handle()` return type inference)
- 2.3 - Buffer copy renames (deprecated old API remains)

### Breaking (Major version bump):
- 3.4 - Queue family abstraction (changes Device API)

---

## Testing Strategy

Each improvement should include:
1. **Unit tests** for new/modified functionality
2. **Negative tests** verifying proper exception throwing
3. **Integration tests** in examples
4. **Documentation** updates in CLAUDE.md

Example test for 1.1:
```cpp
TEST(DeviceTest, PresentQueueThrowsWhenNotConfigured) {
    auto device = createTestDeviceWithoutPresent();
    EXPECT_THROW(device->presentQueue(), vw::LogicException);
}

TEST(DeviceTest, HasPresentQueueReturnsFalseWhenNotConfigured) {
    auto device = createTestDeviceWithoutPresent();
    EXPECT_FALSE(device->has_present_queue());
}
```

---

## Conclusion

The VulkanWrapper library has a solid architecture. These improvements focus on:
1. **Eliminating silent failures** by replacing assertions with exceptions
2. **Making misuse harder** through builder validation and explicit APIs
3. **Improving ergonomics** with convenience builders and clearer naming

The changes are designed to be largely backward-compatible, allowing incremental adoption while maintaining the library's excellent performance characteristics.
