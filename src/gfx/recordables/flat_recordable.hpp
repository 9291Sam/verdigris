#ifndef SRC_GFX_RECORDABLES_FLAT_RECORDABLE_HPP
#define SRC_GFX_RECORDABLES_FLAT_RECORDABLE_HPP

#include "recordable.hpp"
#include <future>
#include <gfx/vulkan/buffer.hpp>
#include <util/threads.hpp>

namespace gfx::recordables
{
    class FlatRecordable final : public Recordable
    {
    public:
        struct Vertex
        {
            glm::vec4 color;
            glm::vec3 position;
            glm::vec3 normal;

            static const vk::VertexInputBindingDescription*
            getBindingDescription();

            static const std::array<vk::VertexInputAttributeDescription, 2>*
            getAttributeDescriptions();
        };

        struct PushConstants
        {
            glm::mat4 model_view_proj;
        };

        using Index = std::uint32_t;
    public:

        static std::shared_ptr<FlatRecordable> create(
            const gfx::Renderer&,
            std::vector<Vertex>,
            std::vector<Index>,
            Transform,
            std::string name);
        ~FlatRecordable() override = default;

        void updateFrameState() const override;
        void record(vk::CommandBuffer, const Camera&) const override;

        util::Mutex<Transform> transform;

    private:
        const vulkan::Pipeline* getPipeline() const;

        mutable std::optional<std::future<gfx::vulkan::Buffer>>
                                                   future_vertex_buffer;
        std::size_t                                number_of_vertices;
        mutable std::optional<gfx::vulkan::Buffer> vertex_buffer;

        mutable std::optional<std::future<gfx::vulkan::Buffer>>
                                                   future_index_buffer;
        std::size_t                                number_of_indices;
        mutable std::optional<gfx::vulkan::Buffer> index_buffer;

        FlatRecordable(
            const gfx::Renderer&,
            std::vector<Vertex>,
            std::vector<Index>,
            Transform,
            std::string name);
    };
} // namespace gfx::recordables

#endif // SRC_GFX_RECORDABLES_FLAT_RECORDABLE_HPP
