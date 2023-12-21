#include "instance.hpp"
#include <GLFW/glfw3.h>
#include <util/log.hpp>

namespace
{
    VkBool32 debugMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        [[maybe_unused]] void*                      pUserData)
    {
        vk::DebugUtilsMessageTypeFlagsEXT type {messageType};
        std::string                       message = pCallbackData->pMessage;

        // discard warnings
        if (message.find(
                "Validation Performance Warning: [ "
                "UNASSIGNED-BestPractices-vkAllocateMemory-small-allocation")
                == 0
            || message.find("Validation Performance Warning: [ "
                            "UNASSIGNED-BestPractices-vkBindMemory-small-"
                            "dedicated-allocation")
                   == 0
            || (VERDIGRIS_ENABLE_VALIDATION
                && message.find("Validation Warning: [ "
                                "UNASSIGNED-BestPractices-vkCreateInstance-"
                                "specialuse-extension-debugging ]")
                       == 0))
        {
            return vk::False;
        }

        util::logWarn(
            "Validation Layer Message: Severity: {} | Type: {} |     {}",
            std::to_underlying(messageSeverity),
            vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT {messageType}),
            message);

        // util::panic("kill");

        return vk::False;
    }
} // namespace

namespace gfx::vulkan
{
    Instance::Instance()
        : dynamic_loader {} // NOLINT
        , instance {nullptr}
        , debug_messenger {nullptr}
        , vulkan_api_version {vk::makeApiVersion(0, 1, 2, 261)}
    {
        {
            util::assertFatal(
                this->dynamic_loader.success(),
                "Vulkan is not supported on this system");

            VULKAN_HPP_DEFAULT_DISPATCHER.init(
                this->dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>(
                    "vkGetInstanceProcAddr"));
        }

        const std::array enabledFeatures {
            vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
            vk::ValidationFeatureEnableEXT::eGpuAssisted,
            vk::ValidationFeatureEnableEXT::eBestPractices,
        };

        // const std::array<vk::ValidationFeatureDisableEXT, 0>
        //     disabledFeatures {};

        //         VkStructureType
        // const void*
        // uint32_t
        // const VkValidationFeatureEnableEXT*
        // uint32_t
        // const VkValidationFeatureDisableEXT*

        const vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo {
            .sType {vk::StructureType::eDebugUtilsMessengerCreateInfoEXT},
            .pNext {},
            .flags {},
            .messageSeverity {
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError},
            .messageType {
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance},
            .pfnUserCallback {debugMessengerCallback},
            .pUserData {nullptr},
        };

        const vk::ValidationFeaturesEXT validationFeaturesCreateInfo {
            .sType {vk::StructureType::eValidationFeaturesEXT},
            .pNext {static_cast<const void*>(&debugUtilsCreateInfo)},
            .enabledValidationFeatureCount {enabledFeatures.size()},
            .pEnabledValidationFeatures {enabledFeatures.data()},
            .disabledValidationFeatureCount {0},
            .pDisabledValidationFeatures {nullptr},
        };

        const vk::ApplicationInfo applicationInfo {
            .sType {vk::StructureType::eApplicationInfo},
            .pNext {nullptr},
            .pApplicationName {"mango"},
            .applicationVersion {VK_MAKE_API_VERSION(
                VERDIGRIS_VERSION_MAJOR,
                VERDIGRIS_VERSION_MINOR,
                VERDIGRIS_VERSION_PATCH,
                VERDIGRIS_VERSION_TWEAK)},
            .pEngineName {"mango"},
            .engineVersion {VK_MAKE_API_VERSION(
                VERDIGRIS_VERSION_MAJOR,
                VERDIGRIS_VERSION_MINOR,
                VERDIGRIS_VERSION_PATCH,
                VERDIGRIS_VERSION_TWEAK)},
            .apiVersion {this->vulkan_api_version},
        };

        const std::vector<const char*> instanceLayers = []
        {
            if constexpr (VERDIGRIS_ENABLE_VALIDATION)
            {
                for (vk::LayerProperties layer :
                     vk::enumerateInstanceLayerProperties())
                {
                    if (std::strcmp(
                            layer.layerName, "VK_LAYER_KHRONOS_validation")
                        == 0)
                    {
                        return std::vector<const char*> {
                            "VK_LAYER_KHRONOS_validation"};
                    }
                }
            }

            return std::vector<const char*> {};
        }();

        const std::vector<const char*> instanceExtensions = []
        {
            std::vector<const char*> temp {};

#ifdef __APPLE__
            temp.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            temp.push_back(
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif // __APPLE__

            if constexpr (VERDIGRIS_ENABLE_VALIDATION)
            {
                temp.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
                temp.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                temp.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
            }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
            const char* const * instanceExtensionsArray    = nullptr;
            std::uint32_t       numberOfInstanceExtensions = 0;

            instanceExtensionsArray =
                glfwGetRequiredInstanceExtensions(&numberOfInstanceExtensions);

            if (instanceExtensionsArray != nullptr)
            {
                // These pointers are alive as long as glfwInit is valid
                for (std::size_t i = 0; i < numberOfInstanceExtensions; ++i)
                {
                    temp.push_back(instanceExtensionsArray[i]); // NOLINT
#pragma clang diagnostic pop
                }
            }

            return temp;
        }();

        const vk::InstanceCreateInfo instanceCreateInfo {
            .sType {vk::StructureType::eInstanceCreateInfo},
            .pNext {[&]
                    {
                        if constexpr (VERDIGRIS_ENABLE_VALIDATION)
                        {
                            return reinterpret_cast<const void*>( // NOLINT
                                &validationFeaturesCreateInfo);
                        }
                        else
                        {
                            return nullptr;
                        }
                    }()},
            .flags {
#ifdef __APPLE__
                vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
#endif // __APPLE__
            },
            .pApplicationInfo {&applicationInfo},
            .enabledLayerCount {
                static_cast<std::uint32_t>(instanceLayers.size())},
            .ppEnabledLayerNames {instanceLayers.data()},
            .enabledExtensionCount {
                static_cast<std::uint32_t>(instanceExtensions.size())},
            .ppEnabledExtensionNames {instanceExtensions.data()},
        };

        this->instance = vk::createInstanceUnique(instanceCreateInfo);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(*this->instance);

        if constexpr (VERDIGRIS_ENABLE_VALIDATION)
        {
            this->debug_messenger =
                this->instance->createDebugUtilsMessengerEXTUnique(
                    debugUtilsCreateInfo);

            util::logLog("Enabled validation layers");
        }
    }

    Instance::~Instance() = default;

    std::uint32_t Instance::getVulkanVersion() const
    {
        return this->vulkan_api_version;
    }

    vk::Instance Instance::operator* () const
    {
        return *this->instance;
    }
} // namespace gfx::vulkan