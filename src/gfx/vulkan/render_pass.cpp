#include "render_pass.hpp"
#include <util/log.hpp>

namespace gfx::vulkan
{
    RenderPass::RenderPass(
        vk::Device               device,
        vk::RenderPassCreateInfo renderPassCreateInfo,
        std::optional<std::pair<vk::Framebuffer, vk::Extent2D>>
                                  maybeFrameBuffer,
        std::span<vk::ClearValue> maybeClearValues)
        : render_pass {device.createRenderPass(renderPassCreateInfo)}
        , framebuffer {maybeFrameBuffer.has_value() ? maybeFrameBuffer->first : nullptr}
        , framebuffer_extent {maybeFrameBuffer.has_value() ? maybeFrameBuffer->second : vk::Extent2D {}}
        , clear_values_size {maybeClearValues.size()}
        , maybe_clear_values {}
    {
        util::assertFatal(
            maybeClearValues.size() <= this->maybe_clear_values.size(),
            "Too many clear values ({}), were supplied!",
            maybeClearValues.size());

        std::copy(
            maybeClearValues.begin(),
            maybeClearValues.end(),
            this->maybe_clear_values.begin());
    }

    void RenderPass::setFramebuffer(
        vk::Framebuffer newFramebuffer, vk::Extent2D newExtent) const
    {
        this->framebuffer        = newFramebuffer;
        this->framebuffer_extent = newExtent;
    }
} // namespace gfx::vulkan
