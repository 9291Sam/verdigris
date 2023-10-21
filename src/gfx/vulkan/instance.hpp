#ifndef SRC_GFX_VULKAN_INSTANCE_HPP
#define SRC_GFX_VULKAN_INSTANCE_HPP

#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace gfx::vulkan
{
    class Instance
    {
    public:

        Instance();
        ~Instance();

        Instance(const Instance&)             = delete;
        Instance(Instance&&)                  = delete;
        Instance& operator= (const Instance&) = delete;
        Instance& operator= (Instance&&)      = delete;

        [[nodiscard]] std::uint32_t getVulkanVersion() const;
        [[nodiscard]] vk::Instance  operator* () const;

    private:
        vk::DynamicLoader                dynamic_loader;
        vk::UniqueInstance               instance;
        vk::UniqueDebugUtilsMessengerEXT debug_messenger;
        std::uint32_t                    vulkan_api_version;
    };
} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_INSTANCE_HPP