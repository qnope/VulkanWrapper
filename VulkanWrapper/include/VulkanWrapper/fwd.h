#pragma once
#include "VulkanWrapper/3rd_party.h"
#include "VulkanWrapper/Memory/BufferUsage.h"

namespace vw {
class SDL_Initializer;
class Window;

class StagingBufferManager;

class Instance;
class PhysicalDevice;
class DeviceFinder;
class Device;
class Queue;
class PresentQueue;
class Image;
class ImageView;
class Sampler;
class CombinedImage;
class Framebuffer;

class Subpass;

class SwapchainBuilder;
class Swapchain;
class Surface;

class ShaderModule;
class ShaderCompiler;
class Pipeline;
class PipelineLayout;
class RayTracingPipeline;

class Semaphore;
class Fence;

class Allocator;
class BufferBase;
template <typename T, bool HostVisible, VkBufferUsageFlags Flags>
class Buffer;

using IndexBuffer = Buffer<uint32_t, false, IndexBufferUsage>;

enum class QueueType;

class DescriptorSetLayout;

class Rendering;
class RenderingBuilder;

namespace Model {
class Mesh;
struct MeshInstance;
class Scene;
namespace Internal {
struct MaterialInfo;
}
namespace Material {
class Material;
class MaterialManager;
class MaterialManagerMap;
} // namespace Material
} // namespace Model

namespace Barrier {
struct ImageState;
struct BufferState;
struct AccelerationStructureState;
class ResourceTracker;
} // namespace Barrier

} // namespace vw
