#ifndef SRC_GFX_VULKAN_GPU_STRUCTURES_HPP
#define SRC_GFX_VULKAN_GPU_STRUCTURES_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::vulkan
{
    using Index = std::uint32_t;

    /// NOTE:
    /// If you change any of these, dont forget to update their
    /// corresponding structs in the shaders!
    struct Vertex
    {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec3 normal;
        glm::vec2 uv;

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

        [[nodiscard]] static const std::
            array<vk::VertexInputAttributeDescription, 4>*
            getAttributeDescriptions()

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
        // [[nodiscard]] std::partial_ordering
        // operator<=> (const Vertex& other) const = default;
    };

    struct ParallaxVertex
    {
        glm::vec3     position;
        std::uint32_t brick_pointer;

        [[nodiscard]] static const vk::VertexInputBindingDescription*
        getBindingDescription()
        {
            static const vk::VertexInputBindingDescription Bindings {
                .binding {0},
                .stride {sizeof(ParallaxVertex)},
                .inputRate {vk::VertexInputRate::eVertex},
            };

            return &Bindings;
        }

        [[nodiscard]] static const std::
            array<vk::VertexInputAttributeDescription, 2>*
            getAttributeDescriptions()

        {
            static const std::array<vk::VertexInputAttributeDescription, 2>
                Descriptions {
                    vk::VertexInputAttributeDescription {
                        .location {0},
                        .binding {0},
                        .format {vk::Format::eR32G32B32Sfloat},
                        .offset {offsetof(ParallaxVertex, position)},
                    },
                    vk::VertexInputAttributeDescription {
                        .location {1},
                        .binding {0},
                        .format {vk::Format::eR32Uint},
                        .offset {offsetof(ParallaxVertex, brick_pointer)},
                    }};

            return &Descriptions;
        }
    };

    struct PushConstants
    {
        glm::mat4 model_view_proj;
    };

    struct ParallaxPushConstants
    {
        glm::mat4 model_view_proj;
        glm::mat4 model;
        glm::vec3 camera_pos;
    };
} // namespace gfx::vulkan

// Vertex hash implementation
namespace std
{
    template<>
    struct hash<gfx::vulkan::Vertex>
    {
        [[nodiscard]] auto
        operator() (const gfx::vulkan::Vertex& vertex) const noexcept -> size_t
        {
            std::size_t      seed {0};
            std::hash<float> hasher;

            auto hashCombine = [](std::size_t& seed_, std::size_t hash_)
            {
                hash_ += 0x9e3779b9 + (seed_ << 6) + (seed_ >> 2);
                seed_ ^= hash_;
            };

            hashCombine(seed, hasher(vertex.position.x));
            hashCombine(seed, hasher(vertex.position.y));
            hashCombine(seed, hasher(vertex.position.z));

            hashCombine(seed, hasher(vertex.color.x));
            hashCombine(seed, hasher(vertex.color.y));
            hashCombine(seed, hasher(vertex.color.z));

            hashCombine(seed, hasher(vertex.normal.x));
            hashCombine(seed, hasher(vertex.normal.y));
            hashCombine(seed, hasher(vertex.normal.z));

            hashCombine(seed, hasher(vertex.uv.x));
            hashCombine(seed, hasher(vertex.uv.y));

            return seed;
        }
    };

} // namespace std

#endif // SRC_GFX_VULKAN_GPU_STRUCTURES_HPP
