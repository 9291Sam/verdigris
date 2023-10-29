#include "render_pass.hpp"
#include "image.hpp"
#include "swapchain.hpp"

namespace gfx::vulkan
{

    RenderPass::RenderPass(
        vk::Device       device,
        const Swapchain& swapchain,
        const Image2D&   depthBuffer)
        : render_pass {nullptr}
    {
        const std::array<vk::AttachmentDescription, 2> attachments {
            vk::AttachmentDescription {
                .flags {},
                .format {swapchain.getSurfaceFormat().format},
                .samples {vk::SampleCountFlagBits::e1},
                .loadOp {vk::AttachmentLoadOp::eClear},
                .storeOp {vk::AttachmentStoreOp::eStore},
                .stencilLoadOp {vk::AttachmentLoadOp::eDontCare},
                .stencilStoreOp {vk::AttachmentStoreOp::eDontCare},
                .initialLayout {vk::ImageLayout::eUndefined},
                .finalLayout {vk::ImageLayout::ePresentSrcKHR},
            },
            vk::AttachmentDescription {
                .flags {},
                .format {depthBuffer.getFormat()},
                .samples {vk::SampleCountFlagBits::e1},
                .loadOp {vk::AttachmentLoadOp::eClear},
                .storeOp {vk::AttachmentStoreOp::eDontCare},
                .stencilLoadOp {vk::AttachmentLoadOp::eDontCare},
                .stencilStoreOp {vk::AttachmentStoreOp::eDontCare},
                .initialLayout {vk::ImageLayout::eUndefined},
                .finalLayout {vk::ImageLayout::eDepthStencilAttachmentOptimal},
            },
        };

        const vk::AttachmentReference colorAttachmentReference {
            .attachment {0},
            .layout {vk::ImageLayout::eColorAttachmentOptimal},
        };

        const vk::AttachmentReference depthAttachmentReference {
            .attachment {1},
            .layout {vk::ImageLayout::eDepthStencilAttachmentOptimal},
        };

        const vk::SubpassDescription subpass {
            .flags {},
            .pipelineBindPoint {
                VULKAN_HPP_NAMESPACE::PipelineBindPoint::eGraphics},
            .inputAttachmentCount {0},
            .pInputAttachments {nullptr},
            .colorAttachmentCount {1},
            .pColorAttachments {&colorAttachmentReference},
            .pResolveAttachments {nullptr},
            .pDepthStencilAttachment {&depthAttachmentReference},
            .preserveAttachmentCount {0},
            .pPreserveAttachments {nullptr},
        };

        const vk::SubpassDependency subpassDependency {
            .srcSubpass {VK_SUBPASS_EXTERNAL},
            .dstSubpass {0},
            .srcStageMask {
                vk::PipelineStageFlagBits::eColorAttachmentOutput
                | vk::PipelineStageFlagBits::eEarlyFragmentTests},
            .dstStageMask {
                vk::PipelineStageFlagBits::eColorAttachmentOutput
                | vk::PipelineStageFlagBits::eEarlyFragmentTests},
            .srcAccessMask {vk::AccessFlagBits::eNone},
            .dstAccessMask {
                vk::AccessFlagBits::eColorAttachmentWrite
                | vk::AccessFlagBits::eDepthStencilAttachmentWrite},
            .dependencyFlags {},
        };

        const vk::RenderPassCreateInfo renderPassCreateInfo {
            .sType {vk::StructureType::eRenderPassCreateInfo},
            .pNext {nullptr},
            .flags {},
            .attachmentCount {attachments.size()},
            .pAttachments {attachments.data()},
            .subpassCount {1},
            .pSubpasses {&subpass},
            .dependencyCount {1},
            .pDependencies {&subpassDependency},
        };

        this->render_pass = device.createRenderPassUnique(renderPassCreateInfo);
    }

    vk::RenderPass RenderPass::operator* () const
    {
        return *this->render_pass;
    }

} // namespace gfx::vulkan
