#ifndef SRC_GFX_RECORDABLES_FLAT_RECORDABLE_HPP
#define SRC_GFX_RECORDABLES_FLAT_RECORDABLE_HPP

#include "recordable.hpp"
#include <boost/functional/hash.hpp>
#include <compare>
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

            constexpr std::partial_ordering
            operator<=> (const Vertex& other) const noexcept
            {
                return std::tie(
                           this->color.r,
                           this->color.g,
                           this->color.b,
                           this->color.a,
                           this->position.x,
                           this->position.y,
                           this->position.z,
                           this->normal.x,
                           this->normal.y,
                           this->normal.z)
                   <=> std::tie(
                           other.color.r,
                           other.color.g,
                           other.color.b,
                           other.color.a,
                           other.position.x,
                           other.position.y,
                           other.position.z,
                           other.normal.x,
                           other.normal.y,
                           other.normal.z);
            }

            constexpr bool operator== (const Vertex& other) const noexcept
            {
                return (*this) <=> other == std::weak_ordering::equivalent;
            }

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
        const gfx::vulkan::Pipeline* getPipeline() const;

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

namespace std
{

    template<>
    struct hash<gfx::recordables::FlatRecordable::Vertex>
    {
        std::size_t operator() (const gfx::recordables::FlatRecordable::Vertex&
                                    vertex) const noexcept
        {
            std::size_t workingHash = std::hash<float> {}(vertex.color.r);

            boost::hash_combine(
                workingHash, std::hash<float> {}(vertex.color.g));
            boost::hash_combine(
                workingHash, std::hash<float> {}(vertex.color.b));
            boost::hash_combine(
                workingHash, std::hash<float> {}(vertex.color.a));

            boost::hash_combine(
                workingHash, std::hash<float> {}(vertex.position.x));
            boost::hash_combine(
                workingHash, std::hash<float> {}(vertex.position.y));
            boost::hash_combine(
                workingHash, std::hash<float> {}(vertex.position.z));

            boost::hash_combine(
                workingHash, std::hash<float> {}(vertex.normal.x));
            boost::hash_combine(
                workingHash, std::hash<float> {}(vertex.normal.y));
            boost::hash_combine(
                workingHash, std::hash<float> {}(vertex.normal.z));

            return workingHash;
        }
    };

} // namespace std

#endif // SRC_GFX_RECORDABLES_FLAT_RECORDABLE_HPP
