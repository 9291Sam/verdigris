#include "debug_menu.hpp"
#include <array>
#include <atomic>
#include <cmath>
#include <gfx/draw_stages.hpp>
#include <gfx/renderer.hpp>
#include <gfx/vulkan/device.hpp>
#include <gfx/vulkan/image.hpp>
#include <gfx/vulkan/instance.hpp>
#include <gfx/vulkan/render_pass.hpp>
#include <gfx/window.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <string_view>
#include <util/log.hpp>
#include <util/uuid.hpp>

namespace
{
    void checkImguiResult(VkResult result)
    {
        util::assertFatal(
            result == VK_SUCCESS,
            "ImGui result check failed! {}",
            vk::to_string(vk::Result {result}));
    }

    std::atomic<bool> isMenuInitalized {false}; // NOLINT
} // namespace

namespace gfx::recordables
{

    std::shared_ptr<DebugMenu> DebugMenu::create(
        const gfx::Renderer&   renderer_,
        gfx::vulkan::Instance& instance_,
        gfx::vulkan::Device&   device_,
        gfx::Window&           window_,
        vk::RenderPass         renderPass)
    {
        std::shared_ptr<DebugMenu> recordable {
            new DebugMenu {renderer_, instance_, device_, window_, renderPass}};

        recordable->registerSelf();

        return recordable;
    }

    DebugMenu::DebugMenu(
        const gfx::Renderer&   renderer_,
        gfx::vulkan::Instance& instance,
        gfx::vulkan::Device&   device,
        gfx::Window&           window,
        vk::RenderPass         renderPass)
        : Recordable {renderer_, "Debug Menu", gfx::DrawStage::DisplayPass, {}}
    {
        util::assertFatal(
            !isMenuInitalized.exchange(true), "Only one DebugMenu can exist!");

        util::logTrace("Creating DebugMenu");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |=                       // NOLINT
            ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |=                       // NOLINT
            ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

        io.IniFilename = nullptr; // disable storing to file

        const vk::DynamicLoader loader;

        // Absolute nastiness, thanks `karnage`!
        auto getFn = [&loader](const char* name)
        {
            return loader.getProcAddress<PFN_vkVoidFunction>(name);
        };
        auto lambda = +[](const char* name, void* ud)
        {
            auto const * gf = reinterpret_cast<decltype(getFn)*>(ud); // NOLINT
            return (*gf)(name);
        };
        ImGui_ImplVulkan_LoadFunctions(lambda, &getFn);

        std::array<vk::DescriptorPoolSize, 11> poolSizes {
            vk::DescriptorPoolSize {
                .type {vk::DescriptorType::eSampler}, .descriptorCount {1000}},
            vk::DescriptorPoolSize {
                .type {vk::DescriptorType::eCombinedImageSampler},
                .descriptorCount {1000}},
            vk::DescriptorPoolSize {
                .type {vk::DescriptorType::eSampledImage},
                .descriptorCount {1000}},
            vk::DescriptorPoolSize {
                .type {vk::DescriptorType::eStorageImage},
                .descriptorCount {1000}},
            vk::DescriptorPoolSize {
                .type {vk::DescriptorType::eUniformTexelBuffer},
                .descriptorCount {1000}},
            vk::DescriptorPoolSize {
                .type {vk::DescriptorType::eStorageTexelBuffer},
                .descriptorCount {1000}},
            vk::DescriptorPoolSize {
                .type {vk::DescriptorType::eUniformBuffer},
                .descriptorCount {1000}},
            vk::DescriptorPoolSize {
                .type {vk::DescriptorType::eStorageBuffer},
                .descriptorCount {1000}},
            vk::DescriptorPoolSize {
                .type {vk::DescriptorType::eUniformBufferDynamic},
                .descriptorCount {1000}},
            vk::DescriptorPoolSize {
                .type {vk::DescriptorType::eStorageBufferDynamic},
                .descriptorCount {1000}},
            vk::DescriptorPoolSize {
                .type {vk::DescriptorType::eInputAttachment},
                .descriptorCount {1000}}};
        // clang-format on

        const vk::DescriptorPoolCreateInfo poolCreateInfo {
            .sType {vk::StructureType::eDescriptorPoolCreateInfo},
            .pNext {nullptr},
            .flags {vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
            .maxSets {6400}, // cry
            .poolSizeCount {static_cast<std::uint32_t>(poolSizes.size())},
            .pPoolSizes {poolSizes.data()},
        };

        this->pool =
            device.asLogicalDevice().createDescriptorPoolUnique(poolCreateInfo);

        ImGui_ImplGlfw_InitForVulkan(window.window, true);

        ImGui_ImplVulkan_InitInfo imguiInitInfo {
            .Instance {*instance},
            .PhysicalDevice {device.asPhysicalDevice()},
            .Device {device.asLogicalDevice()},
            // fun fact, imgui doesn't touch this AND it can't be null,
            // the solution? pass in a dangling pointer!
            .QueueFamily {std::numeric_limits<std::uint32_t>::max()},
            .Queue {
                reinterpret_cast<VkQueue_T*>(0xFFFFFFFFFFFFFFFFULL)}, // NOLINT
            .PipelineCache {nullptr},
            .DescriptorPool {*this->pool},
            .Subpass {0},       // not touched
            .MinImageCount {3}, // >= 2
            .ImageCount {5},    // >= MinImageCount
            .MSAASamples {static_cast<VkSampleCountFlagBits>(
                vk::SampleCountFlagBits::e1)}, // >= VK_SAMPLE_COUNT_1_BIT
                                               // (0 -> default to
                                               // VK_SAMPLE_COUNT_1_BIT)

            // Dynamic Rendering (Optional)
            .UseDynamicRendering {false},
            .ColorAttachmentFormat {},

            // Allocation, Debugging
            .Allocator {nullptr},
            .CheckVkResultFn {checkImguiResult}};

        util::assertFatal(
            ImGui_ImplVulkan_Init(&imguiInitInfo, renderPass),
            "Failed to initalize imguiVulkan");

        const vk::FenceCreateInfo fenceCreateInfo {
            .sType {vk::StructureType::eFenceCreateInfo},
            .pNext {nullptr},
            .flags {},
        };

        vk::UniqueFence imguiFontUploadFence =
            device.asLogicalDevice().createFenceUnique(fenceCreateInfo);

        util::logTrace("Beginning font upload");

        // Upload fonts

        bool result = false;

        do {
            result = device.getMainGraphicsQueue().tryAccess(
                [&](vk::Queue queue, vk::CommandBuffer commandBuffer) -> void
                {
                    const vk::CommandBufferBeginInfo beginInfo {
                        .sType {vk::StructureType::eCommandBufferBeginInfo},
                        .pNext {nullptr},
                        .flags {vk::CommandBufferUsageFlagBits::eOneTimeSubmit},
                        .pInheritanceInfo {nullptr},
                    };

                    commandBuffer.begin(beginInfo);

                    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

                    commandBuffer.end();

                    const vk::SubmitInfo submitInfo {
                        .sType {vk::StructureType::eSubmitInfo},
                        .pNext {nullptr},
                        .waitSemaphoreCount {0},
                        .pWaitSemaphores {nullptr},
                        .pWaitDstStageMask {nullptr},
                        .commandBufferCount {1},
                        .pCommandBuffers {&commandBuffer},
                        .signalSemaphoreCount {0},
                        .pSignalSemaphores {nullptr},
                    };

                    queue.submit(submitInfo, *imguiFontUploadFence);
                });
        }
        while (!result);

        util::logTrace("uploaded fonts");

        auto start = std::chrono::high_resolution_clock::now();

        const vk::Result rresult = device.asLogicalDevice().waitForFences(
            *imguiFontUploadFence, vk::True, ~std::uint64_t {0});

        auto end = std::chrono::high_resolution_clock::now();

        util::logTrace(
            "waitTime  {}ms",
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                .count());
        util::assertFatal(
            rresult == vk::Result::eSuccess,
            "Failed to wait for upload semaphore imgui | {}",
            vk::to_string(rresult));

        ImGui_ImplVulkan_DestroyFontUploadObjects();

        ImGui::StyleColorsDark();
    }

    DebugMenu::~DebugMenu()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        util::assertFatal(
            isMenuInitalized.load(), "Only one DebugMenu can be destroyed");

        util::logDebug("Destroyed DebugMenu");
    }

    void DebugMenu::updateFrameState() const
    {
        this->state = this->renderer.getMenuState().copyInner();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* const viewport = ImGui::GetMainViewport();

        const auto [x, y] = viewport->Size;

        const ImVec2 desiredConsoleSize {2 * x / 9, y}; // 2 / 9 is normal

        ImGui::SetNextWindowPos(
            ImVec2 {std::ceil(viewport->Size.x - desiredConsoleSize.x), 0});
        ImGui::SetNextWindowSize(desiredConsoleSize);

        if (ImGui::Begin(
                "Console",
                nullptr,
                ImGuiWindowFlags_NoResize |            // NOLINT
                    ImGuiWindowFlags_NoSavedSettings | // NOLINT
                    ImGuiWindowFlags_NoMove |          // NOLINT
                    ImGuiWindowFlags_NoDecoration))    // NOLINT
        {
            static constexpr float WindowPadding = 5.0f;

            ImGui::PushStyleVar(
                ImGuiStyleVar_WindowPadding,
                ImVec2(WindowPadding, WindowPadding));
            {
                if (ImGui::Button("Button"))
                {
                    util::logTrace("pressed button");
                }

                if (ImGui::IsItemActive())
                {
                    char random = static_cast<std::string>(util::UUID {})[0];

                    if (random == '\0')
                    {
                        state.string.clear();
                    }
                    else
                    {
                        state.string += random;
                    }
                }

                const std::string playerPosition = std::format(
                    "Player position: {}",
                    glm::to_string(state.player_position));
                ImGui::TextWrapped("%s", playerPosition.c_str());

                const std::string fpsAndTps = std::format(
                    "FPS: {:.3f} | TPS: {:.3f}"
                    " | Frame Time (ms): {:.3f} | ",
                    state.fps,
                    state.tps,
                    1000 / state.fps);

                ImGui::TextWrapped("%s", fpsAndTps.c_str());

                // const float displayImageAspectRatio =
                //     static_cast<float>(this->display_image_size.height)
                //     / static_cast<float>(this->display_image_size.width);

                // const float imageWidth =
                //     std::min(x, desiredConsoleSize.x - WindowPadding *
                //     2); //

                // auto vec =
                //     ImVec2(imageWidth, imageWidth *
                //     displayImageAspectRatio);

                // ImGui::Image(
                //     (ImTextureID)(this->image_descriptor),
                //     ImVec2(imageWidth, imageWidth *
                //     displayImageAspectRatio));
            }

            ImGui::PopStyleVar();

            std::array<std::string_view, 5> loggingLevels {
                "Trace", "Debug", "Log", "Warn", "Fatal"};
            this->current_logging_level_selection =
                // NOLINTNEXTLINE
                loggingLevels[std::to_underlying(util::getCurrentLevel())];

            if (ImGui::BeginCombo(
                    "Logging Level",
                    this->current_logging_level_selection.data()))
            {
                std::size_t idx = 0;
                for (std::string_view s : loggingLevels)
                {
                    bool isSelected =
                        (this->current_logging_level_selection == s);

                    if (ImGui::Selectable(s.data(), isSelected))
                    {
                        this->current_logging_level_selection = s;
                        util::setLoggingLevel(
                            static_cast<util::LoggingLevel>(idx));
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                    idx++;
                }
                ImGui::EndCombo();
            }

            ImGui::End();
        }

        ImGui::Render();
    }

    void DebugMenu::record(
        vk::CommandBuffer commandBuffer,
        vk::PipelineLayout,
        const Camera&) const
    {
        ImDrawData* const maybeDrawData = ImGui::GetDrawData();

        util::assertFatal(
            maybeDrawData != nullptr, "Failed to get ImGui draw data!");

        ImGui_ImplVulkan_RenderDrawData(maybeDrawData, commandBuffer);
    }

    std::pair<vulkan::PipelineCache::PipelineHandle, vk::PipelineBindPoint>
    DebugMenu::getPipeline(const vulkan::PipelineCache&) const
    {
        return {};
    }

    void DebugMenu::setVisibility(bool newVisibility) const
    {
        this->should_draw.store(newVisibility, std::memory_order_release);
    }
} // namespace gfx::recordables
