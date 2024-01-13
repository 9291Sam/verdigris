#include "render_pass.hpp"
#include <engine/settings.hpp>
#include <util/log.hpp>

namespace gfx::vulkan
{
    RenderPass::RenderPass(
        vk::Device               device,
        vk::RenderPassCreateInfo renderPassCreateInfo,
        std::optional<std::pair<vk::Framebuffer, vk::Extent2D>>
                                  maybeFrameBuffer,
        std::span<vk::ClearValue> maybeClearValues,
        const std::string&        name_)
        : render_pass {device.createRenderPassUnique(renderPassCreateInfo)}
        , framebuffer {maybeFrameBuffer.has_value() ? maybeFrameBuffer->first : nullptr}
        , framebuffer_extent {maybeFrameBuffer.has_value() ? maybeFrameBuffer->second : vk::Extent2D {}}
        , clear_values_size {maybeClearValues.size()}
        , maybe_clear_values {}
        , name {name_}
    {
        util::assertFatal(
            maybeClearValues.size() <= this->maybe_clear_values.size(),
            "Too many clear values ({}), were supplied!",
            maybeClearValues.size());

        std::copy(
            maybeClearValues.begin(),
            maybeClearValues.end(),
            this->maybe_clear_values.begin());

        if (engine::getSettings()
                .lookupSetting<engine::Setting::EnableGFXValidation>())
        {
            vk::DebugUtilsObjectNameInfoEXT nameSetInfo {
                .sType {vk::StructureType::eDebugUtilsObjectNameInfoEXT},
                .pNext {nullptr},
                .objectType {vk::ObjectType::eRenderPass},
                .objectHandle {
                    std::bit_cast<std::uint64_t>(*this->render_pass)},
                .pObjectName {this->name.c_str()},
            };

            device.setDebugUtilsObjectNameEXT(nameSetInfo);
        }
    }

    RenderPass::~RenderPass()
    {
        // util::logDebug(
        //     "Destroying {} | {} | {} | {}",
        //     this->name,
        //     (void*)VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyRenderPass,
        //     (void*)&this->render_pass.getDispatch(),
        //     (void*)this->render_pass.getOwner());
    }

    void RenderPass::setFramebuffer(
        vk::Framebuffer newFramebuffer, vk::Extent2D newExtent) const
    {
        this->framebuffer        = newFramebuffer;
        this->framebuffer_extent = newExtent;
    }

    [[nodiscard]] vk::RenderPass RenderPass::operator* () const
    {
        return *this->render_pass;
    }

    [[nodiscard]] vk::Extent2D RenderPass::getExtent() const
    {
        return this->framebuffer_extent;
    }

} // namespace gfx::vulkan
