#include "VulkanWrapper/RenderPass/RenderPass.h"

#include "VulkanWrapper/Image/Framebuffer.h"
#include "VulkanWrapper/RenderPass/Subpass.h"
#include "VulkanWrapper/Utils/Algos.h"
#include "VulkanWrapper/Vulkan/Device.h"

namespace vw {

IRenderPass::IRenderPass(vk::UniqueRenderPass render_pass,
                         std::vector<vk::ClearValue> clear_values)
    : ObjectWithUniqueHandle<vk::UniqueRenderPass>{std::move(render_pass)}
    , m_clear_values{std::move(clear_values)} {}

const std::vector<vk::ClearValue> &IRenderPass::clear_values() const noexcept {
    return m_clear_values;
}

vk::SubpassDescription2
subpassToDescription(const std::unique_ptr<ISubpass> &subpass) {
    const auto result =
        vk::SubpassDescription2()
            .setPipelineBindPoint(subpass->pipeline_bind_point())
            .setColorAttachments(subpass->color_attachments())
            .setInputAttachments(subpass->input_attachments())
            .setPDepthStencilAttachment(subpass->depth_stencil_attachment());

    return result;
}

RenderPassBuilder::RenderPassBuilder(const Device &device)
    : m_device{&device} {}

RenderPassBuilder &&
RenderPassBuilder::add_attachment(vk::AttachmentDescription2 attachment,
                                  vk::ClearValue clear_value) && {
    m_attachments.push_back(attachment);
    m_clear_values.push_back(clear_value);
    return std::move(*this);
}

RenderPassBuilder &&
RenderPassBuilder::add_subpass(SubpassTag tag,
                               std::unique_ptr<ISubpass> subpass) && {
    m_subpasses.emplace_back(tag, std::move(subpass));
    return std::move(*this);
}

vk::UniqueRenderPass RenderPassBuilder::build_underlying() {
    using namespace std::views;
    auto subpasses = m_subpasses | values;
    const auto vkSubpassDescriptions =
        subpasses | transform(subpassToDescription) | to<std::vector>;

    const auto info = vk::RenderPassCreateInfo2()
                          .setAttachments(m_attachments)
                          .setSubpasses(vkSubpassDescriptions)
                          .setDependencies(m_subpass_dependencies);

    auto [result, renderPass] =
        m_device->handle().createRenderPass2Unique(info);

    if (result != vk::Result::eSuccess) {
        throw RenderPassCreationException{std::source_location::current()};
    }

    return std::move(renderPass);
}

RenderPassBuilder &&RenderPassBuilder::add_dependency(SubpassTag src,
                                                      SubpassTag dst) && {
    using namespace std::views;
    auto index_src = index_of(m_subpasses | std::views::keys, src);
    auto index_dst = index_of(m_subpasses | std::views::keys, dst);

    assert(index_src);
    assert(index_dst);

    vk::SubpassDependency2 dependency{};
    auto [src_stage, src_access] =
        m_subpasses[*index_src].second->output_dependencies();
    auto [dst_stage, dst_access] =
        m_subpasses[*index_dst].second->input_dependencies();

    dependency.setSrcSubpass(*index_src)
        .setDstSubpass(*index_dst)
        .setSrcStageMask(src_stage)
        .setSrcAccessMask(src_access)
        .setDstStageMask(dst_stage)
        .setDstAccessMask(dst_access);
    m_subpass_dependencies.push_back(dependency);
    return std::move(*this);
}

} // namespace vw
