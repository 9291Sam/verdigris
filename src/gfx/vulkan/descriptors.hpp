#ifndef SRC_GFX_VULKAN_DESCRIPTORS_HPP
#define SRC_GFX_VULKAN_DESCRIPTORS_HPP

#include <expected>
#include <span>
#include <unordered_map>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::vulkan
{
    class Device;
    class DescriptorSet;
    class DescriptorSetLayout;

    // TODO: have constexpr layouts that can easily be grabbed for pipeline
    // creation

    enum class DescriptorSetType
    {
        None,
        Voxel,
        ComputeRendererOutput,
    };

    struct DescriptorState
    {
        DescriptorState();
        explicit DescriptorState(auto... d)
            : descriptors {d...}
        {}

        void reset();

        std::array<DescriptorSetType, 4> descriptors; // NOLINT
    };

    // TODO: add proper atomics to this
    class DescriptorPool
    {
    public:
        struct AllocationFailure
        {
            std::size_t        tried_to_allocate;
            vk::DescriptorType type;
            std::size_t        number_available;
        };
    public:

        DescriptorPool(
            vk::Device,
            std::unordered_map<vk::DescriptorType, std::uint32_t> capacity);
        ~DescriptorPool();

        DescriptorPool()                                  = delete;
        DescriptorPool(const DescriptorPool&)             = delete;
        DescriptorPool(DescriptorPool&&)                  = delete;
        DescriptorPool& operator= (const DescriptorPool&) = delete;
        DescriptorPool& operator= (DescriptorPool&&)      = delete;

        [[nodiscard]] DescriptorSet allocate(DescriptorSetType);
        DescriptorSetLayout& lookupOrAddLayoutFromCache(DescriptorSetType);

    private:

        friend class DescriptorSet;
        void free(DescriptorSet&);

        vk::Device               device;
        vk::UniqueDescriptorPool pool;
        std::unordered_map<vk::DescriptorType, std::uint32_t>
            inital_descriptors;
        std::unordered_map<vk::DescriptorType, std::uint32_t>
            available_descriptors;

        std::unordered_map<DescriptorSetType, DescriptorSetLayout>
            descriptor_layout_cache;
    }; // class DescriptorPool

    class DescriptorSetLayout
    {
    public:

        DescriptorSetLayout(vk::Device, vk::DescriptorSetLayoutCreateInfo);
        ~DescriptorSetLayout() = default;

        DescriptorSetLayout()                                       = default;
        DescriptorSetLayout(const DescriptorSetLayout&)             = delete;
        DescriptorSetLayout(DescriptorSetLayout&&)                  = default;
        DescriptorSetLayout& operator= (const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator= (DescriptorSetLayout&&)      = default;

        [[nodiscard]] const vk::DescriptorSetLayout* operator* () const;
        [[nodiscard]] std::span<const vk::DescriptorSetLayoutBinding>
        getLayoutBindings() const;

    private:
        vk::UniqueDescriptorSetLayout               layout;
        std::vector<vk::DescriptorSetLayoutBinding> descriptors;
    };

    class DescriptorSet
    {
    public:

        DescriptorSet();
        ~DescriptorSet();

        DescriptorSet(const DescriptorSet&) = delete;
        DescriptorSet(DescriptorSet&&) noexcept;
        DescriptorSet& operator= (const DescriptorSet&) = delete;
        DescriptorSet& operator= (DescriptorSet&&) noexcept;

        [[nodiscard]] vk::DescriptorSet operator* () const;

    private:
        friend class DescriptorPool;
        DescriptorSet(vk::DescriptorSet, DescriptorPool*, DescriptorSetLayout*);

        vk::DescriptorSet    set;
        DescriptorPool*      pool;
        DescriptorSetLayout* layout;
    };

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_DESCRIPTORS_HPP
