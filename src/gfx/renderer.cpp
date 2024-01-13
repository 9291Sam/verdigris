#include "renderer.hpp"
#include "imgui_menu.hpp"
#include "recordables/recordable.hpp"
#include "vulkan/allocator.hpp"
#include "vulkan/device.hpp"
#include "vulkan/frame.hpp"
#include "vulkan/image.hpp"
#include "vulkan/instance.hpp"
#include "vulkan/pipelines.hpp"
#include "vulkan/render_pass.hpp"
#include "vulkan/swapchain.hpp"
#include <GLFW/glfw3.h>
#include <magic_enum_all.hpp>
#include <util/log.hpp>
#include <vk_mem_alloc.h>

namespace gfx
{
    using Action            = gfx::Window::Action;
    using InteractionMethod = gfx::Window::InteractionMethod;
    using ActionInformation = gfx::Window::ActionInformation;

    Renderer::Renderer()
        : window {std::make_unique<Window>(
            std::map<gfx::Window::Action, gfx::Window::ActionInformation> {
                {Action::PlayerMoveForward,
                 ActionInformation {
                     .key {GLFW_KEY_W},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerMoveBackward,
                 ActionInformation {
                     .key {GLFW_KEY_S},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerMoveLeft,
                 ActionInformation {
                     .key {GLFW_KEY_A},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerMoveRight,
                 ActionInformation {
                     .key {GLFW_KEY_D},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerMoveUp,
                 ActionInformation {
                     .key {GLFW_KEY_SPACE},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerMoveDown,
                 ActionInformation {
                     .key {GLFW_KEY_LEFT_CONTROL},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerSprint,
                 ActionInformation {
                     .key {GLFW_KEY_LEFT_SHIFT},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::ToggleConsole,
                 ActionInformation {
                     .key {GLFW_KEY_GRAVE_ACCENT},
                     .method {InteractionMethod::SinglePress}}},

                {Action::ToggleCursorAttachment,
                 ActionInformation {
                     .key {GLFW_KEY_BACKSLASH},
                     .method {InteractionMethod::SinglePress}}},

            },
            vk::Extent2D {1920, 1080}, // NOLINT
            "Verdigris")}
        , instance {std::make_unique<vulkan::Instance>()}
        , surface {std::make_unique<vk::UniqueSurfaceKHR>(
              this->window->createSurface(**this->instance))}
        , device {std::make_unique<vulkan::Device>(
              **this->instance, **this->surface)}
        , allocator {std::make_unique<vulkan::Allocator>(
              *this->instance, &*this->device)}
        , swapchain {nullptr}
        , render_passes {RenderPasses {}}
        , debug_menu {nullptr}
        , draw_camera {Camera {{0.0f, 0.0f, 0.0f}}}
        , is_cursor_attached {true}
    {
        this->initializeRenderer(std::nullopt);

        this->window->attachCursor();

        this->debug_menu->setVisibility(false);
    }

    Renderer::~Renderer() = default;

    float Renderer::getFrameDeltaTimeSeconds() const
    {
        return this->window->getDeltaTimeSeconds();
    }

    Window::Delta Renderer::getMouseDeltaRadians() const
    {
        // each value from -1.0 -> 1.0 representing how much it moved on the
        // screen
        const auto [nDeltaX, nDeltaY] =
            this->window->getScreenSpaceMouseDelta();

        const auto deltaRadiansX = (nDeltaX / 2) * this->getFovXRadians();
        const auto deltaRadiansY = (nDeltaY / 2) * this->getFovYRadians();

        return Window::Delta {.x {deltaRadiansX}, .y {deltaRadiansY}};
    }

    const util::Mutex<recordables::DebugMenu::State>&
    Renderer::getMenuState() const
    {
        return this->debug_menu_state;
    }

    [[nodiscard]] bool Renderer::isActionActive(Window::Action a) const
    {
        return this->window->isActionActive(a);
    }

    float
    gfx::Renderer::getFovYRadians() const // NOLINT: may change in the future
    {
        return glm::radians(70.f); // NOLINT
    }

    float gfx::Renderer::getFovXRadians() const
    {
        // assume that the window is never stretched with an image.
        // i.e 100x pixels are 100y pixels if rotated
        const auto [width, height] = this->window->getFramebufferSize();

        return this->getFovYRadians() * static_cast<float>(width)
             / static_cast<float>(height);
    }

    // float gfx::Renderer::getFocalLength() const
    // {
    //     const auto [width, height] = this->window->getFramebufferSize();
    //     float fovYRadians          = this->getFovYRadians();

    //     // Calculate focal length using the field of view and image size
    //     float focalLength =
    //         static_cast<float>(width) / (2.0f * std::tan(fovYRadians
    //         / 2.0f));

    //     return focalLength;
    // }
    // float gfx::Renderer::getFocalLength() const
    // {
    //     // const auto [width, height] = this->window->getFramebufferSize();
    //     // float fovYRadians          = this->getFovYRadians();

    //     // // Assuming you're using glm::perspective for rasterization
    //     // float focalLength = 1.0f / std::tan(fovYRadians / 2.0f);

    //     // return focalLength;
    //     return 1.0f;
    // }

    float gfx::Renderer::getAspectRatio() const
    {
        const auto [width, height] = this->window->getFramebufferSize();

        return static_cast<float>(width) / static_cast<float>(height);
    }

    void Renderer::setCamera(Camera c)
    {
        this->draw_camera = c;
    }

    bool Renderer::continueTicking()
    {
        return !this->window->shouldClose();
    }

    void Renderer::drawFrame()
    {
        std::expected<void, vulkan::ResizeNeeded> result = [&]
        {
            std::vector<std::shared_ptr<const recordables::Recordable>>
                renderables = [&]
            {
                std::vector<util::UUID> weakRecordablesToRemove {};

                std::vector<std::shared_ptr<const recordables::Recordable>>
                                               strongMaybeDrawRenderables {};
                std::vector<std::future<void>> maybeDrawStateUpdateFutures {};

                // Collect the strong
                for (const auto& [weakUUID, weakRecordable] :
                     this->recordable_registry.access())
                {
                    if (std::shared_ptr<const recordables::Recordable>
                            recordable = weakRecordable.lock())
                    {
                        maybeDrawStateUpdateFutures.push_back(std::async(
                            [rawRecordable = recordable.get()]
                            {
                                rawRecordable->updateFrameState();
                            }));
                        strongMaybeDrawRenderables.push_back(
                            std::move(recordable));
                    }
                    else
                    {
                        weakRecordablesToRemove.push_back(weakUUID);
                    }
                }

                // Purge the weak
                for (const util::UUID& weakID : weakRecordablesToRemove)
                {
                    this->recordable_registry.remove(weakID);
                }

                std::vector<std::shared_ptr<const recordables::Recordable>>
                    strongDrawingRenderables {};

                maybeDrawStateUpdateFutures.clear(); // join all futures

                for (std::shared_ptr<const recordables::Recordable>&
                         maybeDrawingRecordable : strongMaybeDrawRenderables)
                {
                    if (maybeDrawingRecordable->shouldDraw())
                    {
                        strongDrawingRenderables.push_back(
                            std::move(maybeDrawingRecordable));
                    }
                }

                return strongDrawingRenderables;
            }();

            if (this->debug_menu->shouldDraw())
            {
                renderables.push_back(this->debug_menu);
            }

            //! DrawStage is useless because of how all commands in a command
            //! buffer overlap with one another'

            std::map<DrawStage, std::vector<const recordables::Recordable*>>
                stageMap;
            magic_enum::enum_for_each<DrawStage>(
                [&](DrawStage s)
                {
                    stageMap[s] = {};
                });

            for (const std::shared_ptr<const recordables::Recordable>&
                     strongRenderable : renderables)
            {
                stageMap[strongRenderable->getDrawStage()].push_back(
                    &*strongRenderable);
            }

            std::vector<std::pair<
                std::optional<const vulkan::RenderPass*>,
                std::vector<const recordables::Recordable*>>>
                drawRecordables {};

            //! I don't have to say this is bad, this is bad, but it's not a
            //! race!
            this->render_passes.readLock(
                [&](const RenderPasses& renderPasses)
                {
                    for (auto& [stage, recordables] : stageMap)
                    {
                        drawRecordables.push_back(
                            {renderPasses.acquireRenderPassFromStage(stage),
                             std::move(recordables)});
                    }
                });

            // util::logDebug("drawRecordables size: {}",
            // drawRecordables.size()); for (const auto& [maybePass,
            // recordables] : drawRecordables)
            // {
            // util::logDebug(
            //     "Pass: {} | Recordables: {}",
            //     maybePass.has_value() ? (void*)&*maybePass : nullptr,
            //     recordables.size());
            // }

            return this->render_passes.readLock(
                [&](const RenderPasses& renderPasses)
                {
                    return this->frame_manager->renderObjectsFromCamera(
                        this->draw_camera,
                        drawRecordables,
                        *renderPasses.pipeline_cache);
                });
        }();

        util::assertFatal(
            this->debug_menu.use_count() == 1,
            "Debug menu use count was not 0! | Count: {}",
            this->debug_menu.use_count());

        if (!result)
        {
            this->resize();
        }

        if (this->window->isActionActive(Window::Action::ToggleConsole, true))
        {
            this->debug_menu->setVisibility(!this->debug_menu->shouldDraw());
        }

        if (this->window->isActionActive(
                Window::Action::ToggleCursorAttachment, true))
        {
            if (this->is_cursor_attached)
            {
                this->is_cursor_attached = false;
                this->window->detachCursor();
            }
            else
            {
                this->is_cursor_attached = true;
                this->window->attachCursor();
            }
        }

        this->debug_menu_state.lock(
            [&](recordables::DebugMenu::State& state)
            {
                state.fps = 1 / this->getFrameDeltaTimeSeconds();
            });

        this->window->endFrame();
    }

    void Renderer::waitIdle()
    {
        this->device->asLogicalDevice().waitIdle();
    }

    std::optional<const vulkan::RenderPass*>
    Renderer::RenderPasses::acquireRenderPassFromStage(DrawStage stage) const
    {
        switch (stage)
        {
        case DrawStage::TopOfPipeCompute:
            return std::nullopt;
        case DrawStage::VoxelDiscoveryPass:
            return &*this->voxel_discovery_pass;
        case DrawStage::PostDiscoveryCompute:
            return std::nullopt;
        case DrawStage::DisplayPass:
            return &*this->final_raster_pass;
        case DrawStage::PostPipeCompute:
            return std::nullopt;
        }
    }

    void Renderer::resize()
    {
        this->window->blockThisThreadWhileMinimized();
        this->device->asLogicalDevice().waitIdle(); // stall TODO: make better?

        this->render_passes.writeLock(
            [&](RenderPasses& renderPasses)
            {
                // Destroy things that need to be recreated
                {
                    // this->debug_menu.reset();
                    this->frame_manager.reset();

                    renderPasses.pipeline_cache.reset();
                    renderPasses.final_raster_pass.reset();
                    renderPasses.voxel_discovery_image.reset();
                    renderPasses.voxel_discovery_pass.reset();
                    renderPasses.depth_buffer.reset();

                    this->swapchain.reset();
                }

                this->initializeRenderer(&renderPasses);
            });
    }

    void Renderer::initializeRenderer(
        std::optional<RenderPasses*> maybeWriteLockedRenderPass)
    {
        this->swapchain = std::make_unique<vulkan::Swapchain>(
            *this->device, **this->surface, this->window->getFramebufferSize());

        auto renderPassInitFunc = [&](RenderPasses& renderPasses)
        {
            renderPasses.depth_buffer = std::make_unique<vulkan::Image2D>(
                &*this->allocator,
                this->device->asLogicalDevice(),
                this->swapchain->getExtent(),
                vk::Format::eD32Sfloat,
                vk::ImageLayout::eUndefined,
                vk::ImageUsageFlagBits::eDepthStencilAttachment,
                vk::ImageAspectFlagBits::eDepth,
                vk::ImageTiling::eOptimal,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                "Depth buffer");

            renderPasses.voxel_discovery_image =
                std::make_unique<vulkan::Image2D>(
                    &*this->allocator,
                    this->device->asLogicalDevice(),
                    this->swapchain->getExtent(),
                    vk::Format::eR32Sint,
                    vk::ImageLayout::eUndefined,
                    vk::ImageUsageFlagBits::eColorAttachment
                        | vk::ImageUsageFlagBits::eSampled,
                    vk::ImageAspectFlagBits::eColor,
                    vk::ImageTiling::eOptimal,
                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                    "Voxel discovery ID image");

            {
                {
                    const std::array<vk::AttachmentDescription, 2> attachments {
                        vk::AttachmentDescription {
                            .flags {},
                            .format {vk::Format::eR32Sint},
                            .samples {vk::SampleCountFlagBits::e1},
                            .loadOp {vk::AttachmentLoadOp::eClear},
                            .storeOp {vk::AttachmentStoreOp::eStore},
                            .stencilLoadOp {vk::AttachmentLoadOp::eDontCare},
                            .stencilStoreOp {vk::AttachmentStoreOp::eDontCare},
                            .initialLayout {vk::ImageLayout::eUndefined},
                            .finalLayout {
                                vk::ImageLayout::eShaderReadOnlyOptimal},
                        },
                        vk::AttachmentDescription {
                            .flags {},
                            .format {renderPasses.depth_buffer->getFormat()},
                            .samples {vk::SampleCountFlagBits::e1},
                            .loadOp {vk::AttachmentLoadOp::eClear},
                            .storeOp {vk::AttachmentStoreOp::eStore},
                            .stencilLoadOp {vk::AttachmentLoadOp::eDontCare},
                            .stencilStoreOp {vk::AttachmentStoreOp::eDontCare},
                            .initialLayout {vk::ImageLayout::eUndefined},
                            .finalLayout {vk::ImageLayout::
                                              eDepthStencilAttachmentOptimal},
                        },
                    };

                    const vk::AttachmentReference colorAttachmentReference {
                        .attachment {0},
                        .layout {vk::ImageLayout::eColorAttachmentOptimal},
                    };

                    const vk::AttachmentReference depthAttachmentReference {
                        .attachment {1},
                        .layout {
                            vk::ImageLayout::eDepthStencilAttachmentOptimal},
                    };

                    const vk::SubpassDescription subpass {
                        .flags {},
                        .pipelineBindPoint {vk::PipelineBindPoint::eGraphics},
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
                        .srcSubpass {vk::SubpassExternal},
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

                    std::array<vk::ClearValue, 2> clearValues {
                        vk::ClearValue {.color {
                            vk::ClearColorValue {.uint32 {{0U, 0U, 0U, 0U}}}}},
                        vk::ClearValue {
                            .depthStencil {vk::ClearDepthStencilValue {
                                .depth {1.0f}, .stencil {0}}}}};

                    renderPasses.voxel_discovery_pass =
                        std::make_unique<vulkan::RenderPass>(
                            this->device->asLogicalDevice(),
                            renderPassCreateInfo,
                            std::nullopt,
                            clearValues,
                            "Voxel Discovery");
                }

                {
                    std::array<vk::ImageView, 2> attachments {
                        **renderPasses.voxel_discovery_image,
                        **renderPasses.depth_buffer};

                    vk::FramebufferCreateInfo
                        voxelDiscoveryFramebufferCreateInfo {
                            .sType {vk::StructureType::eFramebufferCreateInfo},
                            .pNext {nullptr},
                            .flags {},
                            .renderPass {**renderPasses.voxel_discovery_pass},
                            .attachmentCount {attachments.size()},
                            .pAttachments {attachments.data()},
                            .width {this->swapchain->getExtent().width},
                            .height {this->swapchain->getExtent().height},
                            .layers {1},
                        };

                    renderPasses.voxel_discovery_framebuffer =
                        this->device->asLogicalDevice().createFramebufferUnique(
                            voxelDiscoveryFramebufferCreateInfo);
                }

                renderPasses.voxel_discovery_pass->setFramebuffer(
                    *renderPasses.voxel_discovery_framebuffer,
                    this->swapchain->getExtent());
            }

            // final raster pass
            {
                const std::array<vk::AttachmentDescription, 2> attachments {
                    vk::AttachmentDescription {
                        .flags {},
                        .format {swapchain->getSurfaceFormat().format},
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
                        .format {renderPasses.depth_buffer->getFormat()},
                        .samples {vk::SampleCountFlagBits::e1},
                        .loadOp {vk::AttachmentLoadOp::eLoad},
                        .storeOp {
                            vk::AttachmentStoreOp::eStore}, // why isnt this
                                                            // dont care?
                        .stencilLoadOp {vk::AttachmentLoadOp::eDontCare},
                        .stencilStoreOp {vk::AttachmentStoreOp::eDontCare},
                        .initialLayout {
                            vk::ImageLayout::eDepthStencilAttachmentOptimal},
                        .finalLayout {
                            // why isn't this undefined?
                            vk::ImageLayout::eDepthStencilAttachmentOptimal},
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
                    .pipelineBindPoint {vk::PipelineBindPoint::eGraphics},
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
                    .srcSubpass {vk::SubpassExternal},
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

                std::array<vk::ClearValue, 1> clearValues {
                    vk::ClearValue {.color {vk::ClearColorValue {
                        .float32 {{0.01f, 0.03f, 0.04f, 1.0f}}}}}};

                renderPasses.final_raster_pass =
                    std::make_unique<vulkan::RenderPass>(
                        this->device->asLogicalDevice(),
                        renderPassCreateInfo,
                        std::nullopt,
                        clearValues,
                        "Final Raster");

                renderPasses.final_raster_pass->setFramebuffer(
                    nullptr, this->swapchain->getExtent());
            }
            renderPasses.pipeline_cache =
                std::make_unique<vulkan::PipelineCache>();

            this->frame_manager = std::make_unique<vulkan::FrameManager>(
                &*this->device,
                &*this->swapchain,
                *renderPasses.depth_buffer,
                **renderPasses.final_raster_pass);

            if (this->debug_menu == nullptr)
            {
                this->debug_menu = recordables::DebugMenu::create(
                    *this,
                    *this->instance,
                    *this->device,
                    *this->window,
                    **renderPasses.final_raster_pass);
            }
        };

        if (maybeWriteLockedRenderPass.has_value())
        {
            renderPassInitFunc(**maybeWriteLockedRenderPass);
        }
        else
        {
            this->render_passes.writeLock(renderPassInitFunc);
        }

        util::logTrace("Finished initialization of renderer");
    }

    void Renderer::registerRecordable(
        const std::shared_ptr<const recordables::Recordable>& objectToRegister)
        const
    {
        this->recordable_registry.insert(
            {objectToRegister->getUUID(), std::weak_ptr {objectToRegister}});
    }
} // namespace gfx
