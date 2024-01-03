#include "descriptors.hpp"
#include "util/log.hpp"
#include <engine/settings.hpp>
#include <tuple>
#include <utility>

namespace gfx::vulkan
{
    namespace
    {
        std::unordered_map<vk::DescriptorType, std::atomic<std::uint32_t>>
        makeAtomicMap(
            const std::unordered_map<vk::DescriptorType, std::uint32_t>& map)
        {
            std::unordered_map<vk::DescriptorType, std::atomic<std::uint32_t>>
                out {};

            for (const auto [descriptor, number] : map)
            {
                out.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(descriptor),
                    std::forward_as_tuple(number));
            }

            return out;
        }
    } // namespace

    std::shared_ptr<DescriptorPool> DescriptorPool::create(
        vk::Device                                                   device,
        const std::unordered_map<vk::DescriptorType, std::uint32_t>& capacity)
    {
        return std::shared_ptr<DescriptorPool> {
            new DescriptorPool {device, capacity}};
    }

    DescriptorPool::DescriptorPool(
        vk::Device                                                   device_,
        const std::unordered_map<vk::DescriptorType, std::uint32_t>& capacity)
        : device {device_}
        , pool {nullptr}
        , inital_descriptors {makeAtomicMap(capacity)}
        , available_descriptors {this->inital_descriptors}
    {
        std::vector<vk::DescriptorPoolSize> requestedPoolMembers {};
        requestedPoolMembers.reserve(this->available_descriptors.size());

        for (auto& [descriptor, number] : this->available_descriptors)
        {
            requestedPoolMembers.push_back(vk::DescriptorPoolSize {
                .type {descriptor}, .descriptorCount {number}});
        }

        const vk::DescriptorPoolCreateInfo poolCreateInfo {
            .sType {vk::StructureType::eDescriptorPoolCreateInfo},
            .pNext {nullptr},
            .flags {vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
            .maxSets {256}, //  large number good enough
            .poolSizeCount {
                static_cast<std::uint32_t>(requestedPoolMembers.size())},
            .pPoolSizes {requestedPoolMembers.data()},
        };

        this->pool = this->device.createDescriptorPoolUnique(poolCreateInfo);
    }

    DescriptorPool::~DescriptorPool()
    {
        if (engine::getSettings()
                .lookupSetting<engine::Setting::EnableAppValidation>())
        {
            if (this->available_descriptors != this->inital_descriptors)
            {
                util::logWarn("All descriptors not returned!");

                for (const auto& [initalDescriptorType, initalNum] :
                     this->inital_descriptors)
                {
                    util::logWarn(
                        "Inital #{}, final #{} of type {}",
                        initalNum.load(std::memory_order_acquire),
                        this->available_descriptors.at(initalDescriptorType)
                            .load(std::memory_order_acquire),
                        vk::to_string(initalDescriptorType));
                }
            }
        }
    }

    DescriptorPool::DescriptorHandle DescriptorPool::createNewDescriptorType(
        std::span<vk::DescriptorSetLayoutBinding> bindings) const
    {
        DescriptorHandle handle {
            this->next_handle_id.fetch_add(1, std::memory_order_acq_rel)};

        this->descriptor_cache.insert(
            {handle,
             std::make_shared<DescriptorSetLayout>(DescriptorSetLayout {
                 this->device,
                 vk::DescriptorSetLayoutCreateInfo {
                     .sType {vk::StructureType::eDescriptorSetLayoutCreateInfo},
                     .pNext {nullptr},
                     .flags {},
                     .bindingCount {
                         static_cast<std::uint32_t>(bindings.size())},
                     .pBindings {bindings.data()},
                 }})});

        return handle;
    }

    DescriptorSet DescriptorPool::allocate(DescriptorHandle handle) const
    {
        DescriptorSet allocatedSet {};

        this->descriptor_cache.visit(
            handle,
            [&](std::shared_ptr<DescriptorSetLayout>& cachedLayout)
            {
                // ensure we have enough descriptors available for the
                // desired descriptor set
                for (const auto& binding : cachedLayout->getLayoutBindings())
                {
                    if (this->available_descriptors.at(binding.descriptorType)
                        < binding.descriptorCount)
                    {
                        util::panic(
                            "Unable to allocate {} descriptors of type {} "
                            "from "
                            "a pool with only {} available!",
                            binding.descriptorCount,
                            vk::to_string(binding.descriptorType),
                            this->available_descriptors
                                .at(binding.descriptorType)
                                .load(std::memory_order_acquire));
                    }
                }

                // the allocation will succeed, decrement internal counts
                for (const auto& binding : cachedLayout->getLayoutBindings())
                {
                    const_cast<std::atomic<std::uint32_t>&>( // NOLINT:
                                                             // atomic
                        this->available_descriptors.at(binding.descriptorType))
                        .fetch_sub(
                            binding.descriptorCount, std::memory_order_acq_rel);
                }

                const vk::DescriptorSetAllocateInfo descriptorAllocationInfo {

                    .sType {vk::StructureType::eDescriptorSetAllocateInfo},
                    .pNext {nullptr},
                    .descriptorPool {*this->pool},
                    .descriptorSetCount {1}, // TODO: add array function?
                    .pSetLayouts {**cachedLayout},
                };

                std::vector<vk::DescriptorSet> allocatedDescriptors =
                    this->device.allocateDescriptorSets(
                        descriptorAllocationInfo);

                util::assertFatal(
                    allocatedDescriptors.size() == 1,
                    "Invalid descriptor length returned");

                allocatedSet = DescriptorSet {
                    allocatedDescriptors.at(0),
                    this->weak_from_this(),
                    std::weak_ptr {cachedLayout}};
            });

        return allocatedSet;
    }

    // DescriptorSet DescriptorPool::allocate(DescriptorSetType
    // typeToAllocate)
    // {
    //     DescriptorSetLayout& layout =
    //         this->lookupOrAddLayoutFromCache(typeToAllocate);

    //     // ensure we have enough descriptors available for the desired
    //     // descriptor set
    //     for (const auto& binding : layout.getLayoutBindings())
    //     {
    //         if (this->available_descriptors[binding.descriptorType]
    //             < binding.descriptorCount)
    //         {
    //             util::panic(
    //                 "Unable to allocate {} descriptors of type {} from a
    //                 pool " "with only {} available!",
    //                 binding.descriptorCount,
    //                 vk::to_string(binding.descriptorType),
    //                 this->available_descriptors[binding.descriptorType]);
    //         }
    //     }

    //     // the allocation will succeed decrement internal counts
    //     for (const auto& binding : layout.getLayoutBindings())
    //     {
    //         this->available_descriptors[binding.descriptorType] -=
    //             binding.descriptorCount;
    //     }

    //     const vk::DescriptorSetAllocateInfo descriptorAllocationInfo {

    //         .sType {vk::StructureType::eDescriptorSetAllocateInfo},
    //         .pNext {nullptr},
    //         .descriptorPool {*this->pool},
    //         .descriptorSetCount {1}, // TODO: add array function?
    //         .pSetLayouts {*layout},
    //     };

    //     // TODO: create an array function to deal with multiple
    //     allocation std::vector<vk::DescriptorSet> allocatedDescriptors =
    //         this->device.allocateDescriptorSets(descriptorAllocationInfo);

    //     util::assertFatal(
    //         allocatedDescriptors.size() == 1,
    //         "Invalid descriptor length returned");

    //     return DescriptorSet {allocatedDescriptors.at(0), this, &layout};
    // }

    // DescriptorSetLayout&
    // DescriptorPool::lookupOrAddLayoutFromCache(DescriptorSetType
    // typeToGet)
    // {
    //     if (this->descriptor_layout_cache.contains(typeToGet))
    //     {
    //         return this->descriptor_layout_cache[typeToGet];
    //     }
    //     else
    //     {
    //         switch (typeToGet)
    //         {
    //         case DescriptorSetType::None:
    //             util::panic(
    //                 "Tried to find the layout of a
    //                 DescriptorSetType::None!");
    //             break;
    //         case DescriptorSetType::Voxel: {
    //             std::array<vk::DescriptorSetLayoutBinding, 3> bindings {
    //                 vk::DescriptorSetLayoutBinding {
    //                     .binding {0},
    //                     .descriptorType
    //                     {vk::DescriptorType::eStorageBuffer},
    //                     .descriptorCount {1},
    //                     .stageFlags {vk::ShaderStageFlagBits::eVertex},
    //                     .pImmutableSamplers {nullptr},
    //                 },
    //                 vk::DescriptorSetLayoutBinding {
    //                     .binding {1},
    //                     .descriptorType
    //                     {vk::DescriptorType::eStorageBuffer},
    //                     .descriptorCount {1},
    //                     .stageFlags {vk::ShaderStageFlagBits::eVertex},
    //                     .pImmutableSamplers {nullptr},
    //                 },
    //                 vk::DescriptorSetLayoutBinding {
    //                     .binding {2},
    //                     .descriptorType
    //                     {vk::DescriptorType::eStorageBuffer},
    //                     .descriptorCount {1},
    //                     .stageFlags {vk::ShaderStageFlagBits::eVertex},
    //                     .pImmutableSamplers {nullptr},
    //                 }};

    //             this->descriptor_layout_cache[typeToGet] =
    //             DescriptorSetLayout {
    //                 this->device,
    //                 vk::DescriptorSetLayoutCreateInfo {
    //                     .sType {
    //                         vk::StructureType::eDescriptorSetLayoutCreateInfo},
    //                     .pNext {nullptr},
    //                     .flags {},
    //                     .bindingCount {
    //                         static_cast<std::uint32_t>(bindings.size())},
    //                     .pBindings {bindings.data()},
    //                 }};
    //             break;
    //         }
    //         // TODO: this shouldn't be here, do that thing clark was
    //         talking
    //         // about
    //         case DescriptorSetType::VoxelRayTracing: {
    //             std::array<vk::DescriptorSetLayoutBinding, 4> bindings {
    //                 vk::DescriptorSetLayoutBinding {
    //                     .binding {0},
    //                     .descriptorType
    //                     {vk::DescriptorType::eStorageImage},
    //                     .descriptorCount {1},
    //                     .stageFlags {vk::ShaderStageFlagBits::eCompute},
    //                     .pImmutableSamplers {nullptr},
    //                 },
    //                 vk::DescriptorSetLayoutBinding {
    //                     .binding {1},
    //                     .descriptorType
    //                     {vk::DescriptorType::eUniformBuffer},
    //                     .descriptorCount {1},
    //                     .stageFlags {vk::ShaderStageFlagBits::eCompute},
    //                     .pImmutableSamplers {nullptr},
    //                 },
    //                 vk::DescriptorSetLayoutBinding {
    //                     .binding {2},
    //                     .descriptorType
    //                     {vk::DescriptorType::eStorageBuffer},
    //                     .descriptorCount {1},
    //                     .stageFlags {vk::ShaderStageFlagBits::eCompute},
    //                     .pImmutableSamplers {nullptr},
    //                 },
    //                 vk::DescriptorSetLayoutBinding {
    //                     .binding {3},
    //                     .descriptorType
    //                     {vk::DescriptorType::eStorageBuffer},
    //                     .descriptorCount {1},
    //                     .stageFlags {vk::ShaderStageFlagBits::eCompute},
    //                     .pImmutableSamplers {nullptr},
    //                 },
    //             };

    //             this->descriptor_layout_cache[typeToGet] =
    //             DescriptorSetLayout {
    //                 this->device,
    //                 vk::DescriptorSetLayoutCreateInfo {
    //                     .sType {
    //                         vk::StructureType::eDescriptorSetLayoutCreateInfo},
    //                     .pNext {nullptr},
    //                     .flags {},
    //                     .bindingCount {
    //                         static_cast<std::uint32_t>(bindings.size())},
    //                     .pBindings {bindings.data()},
    //                 }};
    //         }
    //         }

    //         return this->descriptor_layout_cache[typeToGet];
    //     }
    // }

    void DescriptorPool::free(DescriptorSet& setToFree) const
    {
        std::shared_ptr<const DescriptorSetLayout> layout {
            setToFree.layout.lock()};

        util::assertFatal(
            layout != nullptr, "Descriptor Set Layout was out of lifetime!");

        for (const auto& binding : layout->getLayoutBindings())
        {
            const_cast<std::atomic<std::uint32_t>&>( // NOLINT: It's atomic
                this->available_descriptors.at(binding.descriptorType)) +=
                binding.descriptorCount;
        }

        this->device.freeDescriptorSets(*this->pool, *setToFree);
    }

    DescriptorSetLayout::DescriptorSetLayout(
        vk::Device device, vk::DescriptorSetLayoutCreateInfo layoutCreateInfo)
        : layout {nullptr}
    {
        this->descriptors.reserve(layoutCreateInfo.bindingCount);

        for (std::size_t i = 0; i < layoutCreateInfo.bindingCount; ++i)
        {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
            this->descriptors.push_back(
                layoutCreateInfo.pBindings[i]); // NOLINT
#pragma clang diagnostic pop
        }

        this->layout = device.createDescriptorSetLayoutUnique(layoutCreateInfo);
    }

    const vk::DescriptorSetLayout* DescriptorSetLayout::operator* () const
    {
        return &*this->layout;
    }

    std::span<const vk::DescriptorSetLayoutBinding>
    DescriptorSetLayout::getLayoutBindings() const
    {
        return this->descriptors;
    }

    DescriptorSet::DescriptorSet()
        : set {nullptr}
    {}

    DescriptorSet::~DescriptorSet()
    {
        if (this->set != nullptr)
        {
            if (std::shared_ptr strongPool = this->pool.lock())
            {
                strongPool->free(*this);
            }
            else
            {
                util::logWarn(
                    "Unable to free descriptor set as pool was out of "
                    "lifetime!");
            }
        }
    }

    DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
        : set {other.set}
        , pool {other.pool}
        , layout {other.layout}
    {
        other.set    = nullptr;
        other.pool   = {};
        other.layout = {};
    }

    DescriptorSet& DescriptorSet::operator= (DescriptorSet&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        this->~DescriptorSet();

        new (this) DescriptorSet {std::move(other)};

        return *this;
    }

    vk::DescriptorSet DescriptorSet::operator* () const
    {
        return this->set;
    }

    DescriptorSet::DescriptorSet(
        vk::DescriptorSet                        set_,
        std::weak_ptr<const DescriptorPool>      pool_,
        std::weak_ptr<const DescriptorSetLayout> layout_)
        : set {set_}
        , pool {std::move(pool_)}
        , layout {std::move(layout_)}
    {}
} // namespace gfx::vulkan
