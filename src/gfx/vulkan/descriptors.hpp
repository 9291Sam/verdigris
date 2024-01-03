#ifndef SRC_GFX_VULKAN_DESCRIPTORS_HPP
#define SRC_GFX_VULKAN_DESCRIPTORS_HPP

#include <boost/unordered/concurrent_flat_map.hpp>
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

    // TODO: add proper atomics to this
    // TODO: use shared_ptr and child sets hold weak pointers rather than raw
    // ones
    class DescriptorPool : public std::enable_shared_from_this<DescriptorPool>
    {
    public:
        struct AllocationFailure
        {
            std::size_t        tried_to_allocate;
            vk::DescriptorType type;
            std::size_t        number_available;
        };

        struct DescriptorHandle
        {
            std::strong_ordering
            operator<=> (const DescriptorHandle&) const = default;
        private:
            friend class DescriptorPool;

            explicit DescriptorHandle(std::size_t newID)
                : id {newID}
            {}
            std::size_t id;
        };
    public:
        static std::shared_ptr<DescriptorPool> create(
            vk::Device,
            const std::unordered_map<vk::DescriptorType, std::uint32_t>&
                capacity);
        ~DescriptorPool();

        DescriptorPool()                                  = delete;
        DescriptorPool(const DescriptorPool&)             = delete;
        DescriptorPool(DescriptorPool&&)                  = delete;
        DescriptorPool& operator= (const DescriptorPool&) = delete;
        DescriptorPool& operator= (DescriptorPool&&)      = delete;

        [[nodiscard]] DescriptorHandle createNewDescriptorType(
            std::span<vk::DescriptorSetLayoutBinding>) const;

        [[nodiscard]] DescriptorSet allocate(DescriptorHandle) const;
        [[nodiscard]] std::shared_ptr<DescriptorSetLayout>
            getLayout(DescriptorHandle) const;

    private:

        friend class DescriptorSet;
        void free(DescriptorSet&) const;

        vk::Device               device;
        vk::UniqueDescriptorPool pool;
        std::unordered_map<vk::DescriptorType, std::atomic<std::uint32_t>>
            inital_descriptors;
        std::unordered_map<vk::DescriptorType, std::atomic<std::uint32_t>>
            available_descriptors;

        mutable std::atomic<std::size_t> next_handle_id;
        mutable boost::unordered::concurrent_flat_map<
            DescriptorHandle,
            std::shared_ptr<DescriptorSetLayout>>
            descriptor_cache;

        DescriptorPool(
            vk::Device,
            const std::unordered_map<vk::DescriptorType, std::uint32_t>&
                capacity);
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
        DescriptorSet(
            vk::DescriptorSet,
            std::weak_ptr<const DescriptorPool>,
            std::weak_ptr<const DescriptorSetLayout>);

        vk::DescriptorSet                        set;
        std::weak_ptr<const DescriptorPool>      pool;
        std::weak_ptr<const DescriptorSetLayout> layout;
    };

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_DESCRIPTORS_HPP
