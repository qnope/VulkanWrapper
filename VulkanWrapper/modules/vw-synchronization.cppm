module;
#include "VulkanWrapper/3rd_party.h"

export module vw:synchronization;
import "VulkanWrapper/vw_vulkan.h";
import :core;
import :utils;
import :vulkan;

export namespace vw {

// ---- Fence ----

class Fence : public ObjectWithUniqueHandle<vk::UniqueFence> {
    friend class FenceBuilder;
    friend class Queue;

  public:
    void wait() const;
    void reset() const;

    Fence(const Fence &) = delete;
    Fence(Fence &&) noexcept = default;
    Fence &operator=(const Fence &) = delete;
    Fence &operator=(Fence &&) noexcept = default;
    ~Fence();

  private:
    Fence(vk::Device device, vk::UniqueFence fence);

    vk::Device m_device;
};

class FenceBuilder {
    friend class Queue;
    FenceBuilder(vk::Device device);
    Fence build();

  private:
    vk::Device m_device;
};

// ---- Semaphore ----

class Semaphore : public ObjectWithUniqueHandle<vk::UniqueSemaphore> {
    friend class SemaphoreBuilder;

  public:
  private:
    using ObjectWithUniqueHandle<vk::UniqueSemaphore>::ObjectWithUniqueHandle;
};

class SemaphoreBuilder {
  public:
    SemaphoreBuilder(std::shared_ptr<const Device> device);
    Semaphore build();

  private:
    std::shared_ptr<const Device> m_device;
};

// ---- Interval ----

struct BufferInterval {
    vk::DeviceSize offset;
    vk::DeviceSize size;

    BufferInterval()
        : offset(0)
        , size(0) {}
    BufferInterval(vk::DeviceSize offset, vk::DeviceSize size)
        : offset(offset)
        , size(size) {}

    [[nodiscard]] vk::DeviceSize end() const { return offset + size; }
    [[nodiscard]] bool empty() const { return size == 0; }
    [[nodiscard]] bool contains(const BufferInterval &other) const;
    [[nodiscard]] bool overlaps(const BufferInterval &other) const;
    [[nodiscard]] std::optional<BufferInterval>
    merge(const BufferInterval &other) const;
    [[nodiscard]] std::optional<BufferInterval>
    intersect(const BufferInterval &other) const;
    [[nodiscard]] std::vector<BufferInterval>
    difference(const BufferInterval &other) const;

    bool operator==(const BufferInterval &other) const {
        return offset == other.offset && size == other.size;
    }
    bool operator!=(const BufferInterval &other) const {
        return !(*this == other);
    }
};

struct ImageInterval {
    vk::ImageSubresourceRange range;

    ImageInterval()
        : range() {}
    explicit ImageInterval(const vk::ImageSubresourceRange &range)
        : range(range) {}

    [[nodiscard]] bool empty() const {
        return range.layerCount == 0 || range.levelCount == 0;
    }
    [[nodiscard]] bool contains(const ImageInterval &other) const;
    [[nodiscard]] bool overlaps(const ImageInterval &other) const;
    [[nodiscard]] std::optional<ImageInterval>
    merge(const ImageInterval &other) const;
    [[nodiscard]] std::optional<ImageInterval>
    intersect(const ImageInterval &other) const;
    [[nodiscard]] std::vector<ImageInterval>
    difference(const ImageInterval &other) const;

    bool operator==(const ImageInterval &other) const {
        return range.aspectMask == other.range.aspectMask &&
               range.baseMipLevel == other.range.baseMipLevel &&
               range.levelCount == other.range.levelCount &&
               range.baseArrayLayer == other.range.baseArrayLayer &&
               range.layerCount == other.range.layerCount;
    }
    bool operator!=(const ImageInterval &other) const {
        return !(*this == other);
    }
};

// ---- IntervalSet ----

class BufferIntervalSet {
  public:
    BufferIntervalSet() = default;

    void add(const BufferInterval &interval);
    void remove(const BufferInterval &interval);

    [[nodiscard]] std::vector<BufferInterval>
    findOverlapping(const BufferInterval &interval) const;
    [[nodiscard]] bool hasOverlap(const BufferInterval &interval) const;
    [[nodiscard]] const std::vector<BufferInterval> &intervals() const {
        return m_intervals;
    }
    [[nodiscard]] bool empty() const { return m_intervals.empty(); }
    [[nodiscard]] size_t size() const { return m_intervals.size(); }
    void clear() { m_intervals.clear(); }

  private:
    std::vector<BufferInterval> m_intervals;
    void mergeSorted();
};

class ImageIntervalSet {
  public:
    ImageIntervalSet() = default;

    void add(const ImageInterval &interval);
    void remove(const ImageInterval &interval);

    [[nodiscard]] std::vector<ImageInterval>
    findOverlapping(const ImageInterval &interval) const;
    [[nodiscard]] bool hasOverlap(const ImageInterval &interval) const;
    [[nodiscard]] const std::vector<ImageInterval> &intervals() const {
        return m_intervals;
    }
    [[nodiscard]] bool empty() const { return m_intervals.empty(); }
    [[nodiscard]] size_t size() const { return m_intervals.size(); }
    void clear() { m_intervals.clear(); }

  private:
    std::vector<ImageInterval> m_intervals;
    void mergeCompatible();
};

} // namespace vw

// ---- ResourceTracker (Barrier namespace) ----

export namespace vw::Barrier {

struct ImageState {
    vk::Image image;
    vk::ImageSubresourceRange subresourceRange;
    vk::ImageLayout layout;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

struct BufferState {
    vk::Buffer buffer;
    vk::DeviceSize offset;
    vk::DeviceSize size;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

struct AccelerationStructureState {
    vk::AccelerationStructureKHR handle;
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
};

using ResourceState =
    std::variant<ImageState, BufferState, AccelerationStructureState>;

class ResourceTracker {
  public:
    ResourceTracker() = default;

    void track(const ResourceState &state);
    void request(const ResourceState &state);
    void flush(vk::CommandBuffer commandBuffer);

    // Inspection accessors (used by tests)
    [[nodiscard]] const std::vector<vk::BufferMemoryBarrier2> &
    pending_buffer_barriers() const noexcept {
        return m_pending_buffer_barriers;
    }
    [[nodiscard]] const std::vector<vk::ImageMemoryBarrier2> &
    pending_image_barriers() const noexcept {
        return m_pending_image_barriers;
    }
    [[nodiscard]] const std::vector<vk::MemoryBarrier2> &
    pending_memory_barriers() const noexcept {
        return m_pending_memory_barriers;
    }
    void clear_pending_barriers() noexcept {
        m_pending_buffer_barriers.clear();
        m_pending_image_barriers.clear();
        m_pending_memory_barriers.clear();
    }

    struct BufferStateInfo {
        BufferInterval interval;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
    };
    [[nodiscard]] std::vector<BufferStateInfo>
    buffer_states_for(vk::Buffer buffer) const;

    struct ImageStateInfo {
        ImageInterval interval;
        vk::ImageLayout layout;
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
    };
    [[nodiscard]] std::vector<ImageStateInfo>
    image_states_for(vk::Image image) const;

  private:
    struct InternalImageState {
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;
        vk::PipelineStageFlags2 stage = vk::PipelineStageFlagBits2::eNone;
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;
    };

    struct InternalBufferState {
        vk::PipelineStageFlags2 stage = vk::PipelineStageFlagBits2::eNone;
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;
    };

    struct InternalAccelerationStructureState {
        vk::PipelineStageFlags2 stage = vk::PipelineStageFlagBits2::eNone;
        vk::AccessFlags2 access = vk::AccessFlagBits2::eNone;
    };

    struct HandleHash {
        std::size_t
        operator()(const vk::AccelerationStructureKHR &handle) const {
            return std::hash<uint64_t>()(
                (uint64_t)(VkAccelerationStructureKHR)handle);
        }
    };

    struct BufferHandleHash {
        std::size_t operator()(const vk::Buffer &buffer) const {
            return std::hash<uint64_t>()((uint64_t)(VkBuffer)buffer);
        }
    };

    struct ImageHandleHash {
        std::size_t operator()(const vk::Image &image) const {
            return std::hash<uint64_t>()((uint64_t)(VkImage)image);
        }
    };

    struct BufferIntervalSetState {
        vw::BufferIntervalSet intervals;
        InternalBufferState state;
    };

    struct ImageIntervalSetState {
        vw::ImageIntervalSet intervals;
        InternalImageState state;
    };

    std::unordered_map<vk::Buffer, std::vector<BufferIntervalSetState>,
                       BufferHandleHash>
        m_buffer_states;
    std::unordered_map<vk::Image, std::vector<ImageIntervalSetState>,
                       ImageHandleHash>
        m_image_states;
    std::unordered_map<vk::AccelerationStructureKHR,
                       InternalAccelerationStructureState, HandleHash>
        m_as_states;

    std::vector<vk::ImageMemoryBarrier2> m_pending_image_barriers;
    std::vector<vk::BufferMemoryBarrier2> m_pending_buffer_barriers;
    std::vector<vk::MemoryBarrier2> m_pending_memory_barriers;

    void track_image(vk::Image image,
                     vk::ImageSubresourceRange subresourceRange,
                     vk::ImageLayout layout, vk::PipelineStageFlags2 stage,
                     vk::AccessFlags2 access);
    void track_buffer(vk::Buffer buffer, vk::DeviceSize offset,
                      vk::DeviceSize size, vk::PipelineStageFlags2 stage,
                      vk::AccessFlags2 access);
    void track_acceleration_structure(vk::AccelerationStructureKHR handle,
                                      vk::PipelineStageFlags2 stage,
                                      vk::AccessFlags2 access);

    void request_image(vk::Image image,
                       vk::ImageSubresourceRange subresourceRange,
                       vk::ImageLayout layout, vk::PipelineStageFlags2 stage,
                       vk::AccessFlags2 access);
    void request_buffer(vk::Buffer buffer, vk::DeviceSize offset,
                        vk::DeviceSize size, vk::PipelineStageFlags2 stage,
                        vk::AccessFlags2 access);
    void request_acceleration_structure(vk::AccelerationStructureKHR handle,
                                        vk::PipelineStageFlags2 stage,
                                        vk::AccessFlags2 access);
};

} // namespace vw::Barrier
