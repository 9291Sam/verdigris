#include "imgui_menu.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "util/log.hpp"
#include "util/uuid.hpp"
#include "vulkan/device.hpp"
#include "vulkan/render_pass.hpp"
#include "window.hpp"
#include <atomic>
#include <cmath>
#include <glm/gtx/string_cast.hpp>

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

namespace gfx
{

    ImGuiMenu::ImGuiMenu(
        const Window&         window,
        vk::Instance          instance,
        const vulkan::Device& device,
        vk::RenderPass        renderPass)
        : pool {nullptr}
        , sampler {nullptr}
        , image_descriptor {nullptr}
    {
        util::assertFatal(
            !isMenuInitalized.exchange(true), "Only one ImGuiMenu can exist!");

        util::logTrace("Creating ImguiMenu");
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
        auto get_fn = [&loader](const char* name)
        {
            return loader.getProcAddress<PFN_vkVoidFunction>(name);
        };
        auto lambda = +[](const char* name, void* ud)
        {
            auto const * gf = reinterpret_cast<decltype(get_fn)*>(ud); // NOLINT
            return (*gf)(name);
        };
        ImGui_ImplVulkan_LoadFunctions(lambda, &get_fn);

        // TODO: trim this
        // clang-format off
    std::array<vk::DescriptorPoolSize, 11> PoolSizes {
        vk::DescriptorPoolSize {.type {vk::DescriptorType::eSampler}, .descriptorCount {1000}},
        vk::DescriptorPoolSize {.type {vk::DescriptorType::eCombinedImageSampler}, .descriptorCount {1000}},
        vk::DescriptorPoolSize {.type {vk::DescriptorType::eSampledImage}, .descriptorCount {1000}},
        vk::DescriptorPoolSize {.type {vk::DescriptorType::eStorageImage}, .descriptorCount {1000}},
        vk::DescriptorPoolSize {.type {vk::DescriptorType::eUniformTexelBuffer}, .descriptorCount {1000}},
        vk::DescriptorPoolSize {.type {vk::DescriptorType::eStorageTexelBuffer}, .descriptorCount {1000}},
        vk::DescriptorPoolSize {.type {vk::DescriptorType::eUniformBuffer}, .descriptorCount {1000}},
        vk::DescriptorPoolSize {.type {vk::DescriptorType::eStorageBuffer}, .descriptorCount {1000}},
        vk::DescriptorPoolSize {.type {vk::DescriptorType::eUniformBufferDynamic}, .descriptorCount {1000}},
        vk::DescriptorPoolSize {.type {vk::DescriptorType::eStorageBufferDynamic}, .descriptorCount {1000}},
        vk::DescriptorPoolSize {.type {vk::DescriptorType::eInputAttachment}, .descriptorCount {1000}}};
        // clang-format on

        const vk::DescriptorPoolCreateInfo poolCreateInfo {
            .sType {vk::StructureType::eDescriptorPoolCreateInfo},
            .pNext {nullptr},
            .flags {vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
            .maxSets {6400}, // cry
            .poolSizeCount {static_cast<std::uint32_t>(PoolSizes.size())},
            .pPoolSizes {PoolSizes.data()},
        };

        this->pool =
            device.asLogicalDevice().createDescriptorPoolUnique(poolCreateInfo);

        vk::SamplerCreateInfo samplerCreateInfo {
            .sType {vk::StructureType::eSamplerCreateInfo},
            .pNext {nullptr},
            .flags {},
            .magFilter {vk::Filter::eLinear},
            .minFilter {vk::Filter::eLinear},
            .mipmapMode {vk::SamplerMipmapMode::eLinear},
            .addressModeU {vk::SamplerAddressMode::eRepeat},
            .addressModeV {vk::SamplerAddressMode::eRepeat},
            .addressModeW {vk::SamplerAddressMode::eRepeat},
            .mipLodBias {},
            .anisotropyEnable {static_cast<vk::Bool32>(false)},
            .maxAnisotropy {1.0f},
            .compareEnable {static_cast<vk::Bool32>(false)},
            .compareOp {vk::CompareOp::eNever},
            .minLod {-1000},
            .maxLod {1000},
            .borderColor {vk::BorderColor::eFloatTransparentBlack},
            .unnormalizedCoordinates {},
        };

        this->sampler =
            device.asLogicalDevice().createSamplerUnique(samplerCreateInfo);

        ImGui_ImplGlfw_InitForVulkan(window.window, true);

        ImGui_ImplVulkan_InitInfo imgui_init_info {
            .Instance {instance},
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
            .MinImageCount {2}, // >= 2
            .ImageCount {2},    // >= MinImageCount
            .MSAASamples {static_cast<VkSampleCountFlagBits>(
                vk::SampleCountFlagBits::e1)}, // >= VK_SAMPLE_COUNT_1_BIT (0 ->
                                               // default to
                                               // VK_SAMPLE_COUNT_1_BIT)

            // Dynamic Rendering (Optional)
            .UseDynamicRendering {false},
            .ColorAttachmentFormat {},

            // Allocation, Debugging
            .Allocator {nullptr},
            .CheckVkResultFn {checkImguiResult}};

        util::assertFatal(
            ImGui_ImplVulkan_Init(&imgui_init_info, renderPass),
            "Failed to initalize imguiVulkan");

        const vk::FenceCreateInfo fenceCreateInfo {
            .sType {vk::StructureType::eFenceCreateInfo},
            .pNext {nullptr},
            .flags {},
        };

        vk::UniqueFence imguiFontUploadFence =
            device.asLogicalDevice().createFenceUnique(fenceCreateInfo);

        // Upload fonts
        device.accessQueue(
            vk::QueueFlagBits::eGraphics,
            [&](vk::Queue queue, vk::CommandBuffer commandBuffer) -> void
            {
                const vk::CommandBufferBeginInfo BeginInfo {
                    .sType {vk::StructureType::eCommandBufferBeginInfo},
                    .pNext {nullptr},
                    .flags {vk::CommandBufferUsageFlagBits::eOneTimeSubmit},
                    .pInheritanceInfo {nullptr},
                };

                commandBuffer.begin(BeginInfo);

                ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

                commandBuffer.end();

                const vk::SubmitInfo SubmitInfo {
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

                queue.submit(SubmitInfo, *imguiFontUploadFence);
            });

        const vk::Result result = device.asLogicalDevice().waitForFences(
            *imguiFontUploadFence, static_cast<vk::Bool32>(true), -1);

        util::assertFatal(
            result == vk::Result::eSuccess,
            "Failed to wait for upload semaphore imgui | {}",
            vk::to_string(result));

        ImGui_ImplVulkan_DestroyFontUploadObjects();

        ImGui::StyleColorsDark();
    }

    ImGuiMenu::~ImGuiMenu() noexcept
    {
        if (this->image_descriptor != nullptr)
        {
            ImGui_ImplVulkan_RemoveTexture(this->image_descriptor);
        }

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        util::assertFatal(
            isMenuInitalized.exchange(false),
            "Only one ImGuiMenu can be destroyed");
    }

    void ImGuiMenu::bindImage(vk::ImageView viewToBind)
    {
        this->image_descriptor = ImGui_ImplVulkan_AddTexture(
            *this->sampler,
            viewToBind,
            static_cast<VkImageLayout>(
                vk::ImageLayout::eShaderReadOnlyOptimal));
    }

    void ImGuiMenu::render(State& state) // NOLINT
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* const Viewport = ImGui::GetMainViewport();

        const auto [x, y] = Viewport->Size;

        const ImVec2 DesiredConsoleSize {2 * x / 9, y};

        ImGui::SetNextWindowPos(
            ImVec2 {std::ceil(Viewport->Size.x - DesiredConsoleSize.x), 0});
        ImGui::SetNextWindowSize(DesiredConsoleSize);

        if (ImGui::Begin(
                "Console",
                nullptr,
                ImGuiWindowFlags_NoResize |            // NOLINT
                    ImGuiWindowFlags_NoSavedSettings | // NOLINT
                    ImGuiWindowFlags_NoMove |          // NOLINT
                    ImGuiWindowFlags_NoDecoration))    // NOLINT
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
                "Player position: {}", glm::to_string(state.player_position));
            ImGui::TextUnformatted(playerPosition.c_str());

            const std::string fpsAndTps =
                std::format("FPS: {:.3f} | TPS: {:.3f}", state.fps, state.tps);

            ImGui::TextUnformatted(fpsAndTps.c_str());

            const std::string s = state.string;
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(state.string.c_str());
            ImGui::PopTextWrapPos();

            ImGui::Image((ImTextureID)this->image_descriptor, ImVec2(256, 256));

            ImGui::End();
        }

        ImGui::Render();
    }

    void ImGuiMenu::draw(vk::CommandBuffer commandBuffer) // NOLINT
    {
        ImDrawData* const maybeDrawData = ImGui::GetDrawData();

        util::assertFatal(
            maybeDrawData != nullptr, "Failed to get ImGui draw data!");

        ImGui_ImplVulkan_RenderDrawData(maybeDrawData, commandBuffer);
    }
} // namespace gfx
