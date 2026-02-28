export module vw.vulkan:mipmap;
import std3rd;
import vulkan3rd;
import :image;

export namespace vw {

void generate_mipmap(vk::CommandBuffer cmd_buffer,
                     const std::shared_ptr<const Image> &image);

} // namespace vw
