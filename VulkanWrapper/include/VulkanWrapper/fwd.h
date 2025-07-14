#pragma once

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

class ISubpass;
class IRenderPass;

template <typename> class Subpass;

template <typename> class RenderPass;

class SwapchainBuilder;
class Swapchain;
class Surface;

class ShaderModule;
class Pipeline;
class PipelineLayout;
class RayTracingPipeline;

class Semaphore;
class Fence;

class Allocator;

enum class QueueType;

class DescriptorSetLayout;

namespace Model {
class Mesh;
namespace Internal {
struct MaterialInfo;
}
namespace Material {
class Material;
class MaterialManager;
class MaterialManagerMap;
} // namespace Material
} // namespace Model
} // namespace vw
