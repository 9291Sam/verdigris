#ifndef SRC_GFX_VULKAN_GPU_STRUCTURES_HPP
#define SRC_GFX_VULKAN_GPU_STRUCTURES_HPP

#include <util/matrix.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::vulkan
{
    using Index = std::uint32_t;

    template<class T, std::size_t N>
    concept VulkanVertex = requires(const T t) {
        {
            T::getBindingDescription
        } -> std::same_as<const vk::VertexInputBindingDescription*>;

        {
            T::getAttributeDescriptions
        } -> std::same_as<
            const std::array<vk::VertexInputAttributeDescription, N>*>;

        {
            t.operator std::string ()
        } -> std::same_as<std::string>;

        {
            t == t
        } -> std::same_as<bool>;

        {
            t <=> t
        } -> std::same_as<std::partial_ordering>;

        {
            std::is_trivially_copyable_v<T>
        };
    };

    /// NOTE:
    /// If you change any of these, dont forget to update their
    /// corresponding structs in the shaders!
    struct Vertex
    {
        util::Vec3 position;
        util::Vec4 color;
        util::Vec3 normal;
        util::Vec2 uv;

        [[nodiscard]] static const vk::VertexInputBindingDescription*
        getBindingDescription()
        {
            static const vk::VertexInputBindingDescription bindings {
                .binding {0},
                .stride {sizeof(Vertex)},
                .inputRate {vk::VertexInputRate::eVertex},
            };

            return &bindings;
        }

        [[nodiscard]] static auto getAttributeDescriptions()
            -> const std::array<vk::VertexInputAttributeDescription, 4>*
        {
            // clang-format off
            static const std::array<vk::VertexInputAttributeDescription, 4>
            descriptions
            {
                vk::VertexInputAttributeDescription
                {
                    .location {0},
                    .binding {0},
                    .format {vk::Format::eR32G32B32Sfloat},
                    .offset {offsetof(Vertex, position)},
                },
                vk::VertexInputAttributeDescription
                {
                    .location {1},
                    .binding {0},
                    .format {vk::Format::eR32G32B32A32Sfloat},
                    .offset {offsetof(Vertex, color)},
                },
                vk::VertexInputAttributeDescription
                {
                    .location {2},
                    .binding {0},
                    .format {vk::Format::eR32G32B32Sfloat},
                    .offset {offsetof(Vertex, normal)},
                },
                vk::VertexInputAttributeDescription
                {
                    .location {3},
                    .binding {0},
                    .format {vk::Format::eR32G32Sfloat},
                    .offset {offsetof(Vertex, uv)},
                },
            };
            // clang-format on
            return &descriptions;
        }

        [[nodiscard]] bool operator== (const Vertex&) const = default;
        [[nodiscard]] std::partial_ordering
        operator<=> (const Vertex& other) const = default;
    };

    struct PushConstants
    {
        util::Mat4 model_view_proj;
    };
} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_GPU_STRUCTURES_HPP