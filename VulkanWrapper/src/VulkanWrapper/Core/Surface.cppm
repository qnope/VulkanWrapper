export module vw.vulkan:surface;
import vulkan3rd;
import vw.utils;

export namespace vw {
class Surface : public ObjectWithUniqueHandle<vk::UniqueSurfaceKHR> {
  public:
    Surface(vk::UniqueSurfaceKHR surface) noexcept;
};
} // namespace vw
