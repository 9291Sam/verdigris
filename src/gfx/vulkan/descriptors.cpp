#include "descriptors.hpp"
#include "device.hpp"
#include "util/log.hpp"
#include <map>

namespace gfx::vulkan
{

    std::shared_ptr<DescriptorSetLayout> getDescriptorSetLayout(
        DescriptorSetType type, std::shared_ptr<Device> device)
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
        static std::map<DescriptorSetType, std::shared_ptr<DescriptorSetLayout>>
            cache;
#pragma clang diagnostic pop

        if (cache.contains(type))
        {
            return cache[type];
        }
        else
        {
            switch (type)
            {
            case DescriptorSetType::None:
                util::panic(
                    "Tried to find the layout of a DescriptorSetType::None!");
                break;
            case DescriptorSetType::Voxel:

                std::array<vk::DescriptorSetLayoutBinding, 3> bindings {
                    vk::DescriptorSetLayoutBinding {
                        .binding {0},
                        .descriptorType {vk::DescriptorType::eStorageBuffer},
                        .descriptorCount {1},
                        .stageFlags {vk::ShaderStageFlagBits::eVertex},
                        .pImmutableSamplers {nullptr},
                    },
                    vk::DescriptorSetLayoutBinding {
                        .binding {1},
                        .descriptorType {vk::DescriptorType::eStorageBuffer},
                        .descriptorCount {1},
                        .stageFlags {vk::ShaderStageFlagBits::eVertex},
                        .pImmutableSamplers {nullptr},
                    },
                    vk::DescriptorSetLayoutBinding {
                        .binding {2},
                        .descriptorType {vk::DescriptorType::eStorageBuffer},
                        .descriptorCount {1},
                        .stageFlags {vk::ShaderStageFlagBits::eVertex},
                        .pImmutableSamplers {nullptr},
                    }};

                cache[type] = std::make_shared<DescriptorSetLayout>(
                    std::move(device),
                    vk::DescriptorSetLayoutCreateInfo {
                        .sType {
                            vk::StructureType::eDescriptorSetLayoutCreateInfo},
                        .pNext {nullptr},
                        .flags {},
                        .bindingCount {
                            static_cast<std::uint32_t>(bindings.size())},
                        .pBindings {bindings.data()},
                    });
                break;
            }

            return cache[type];
        }
    }

    DescriptorState::DescriptorState()
        : descriptors {{
            DescriptorSetType::None,
            DescriptorSetType::None,
            DescriptorSetType::None,
            DescriptorSetType::None,
        }}
    {}

    void DescriptorState::reset()
    {
        for (auto& d : descriptors)
        {
            d = DescriptorSetType::None;
        }
    }

    std::shared_ptr<DescriptorPool> DescriptorPool::create(
        std::shared_ptr<Device>                               device,
        std::unordered_map<vk::DescriptorType, std::uint32_t> capacity)
    {
        // We need to access a private constructor, std::make_shared won't work
        return std::shared_ptr<DescriptorPool>(
            new DescriptorPool {std::move(device), std::move(capacity)});
    }

    DescriptorSet
    DescriptorPool::allocate(std::shared_ptr<DescriptorSetLayout> layout)
    {
        // ensure we have enough descriptors available for the desired
        // descriptor set
        for (const auto& binding : layout->getLayoutBindings())
        {
            if (this->available_descriptors[binding.descriptorType]
                < binding.descriptorCount)
            {
                util::panic(
                    "Unable to allocate {} descriptors of type {} from a pool "
                    "with only {} available!",
                    binding.descriptorCount,
                    vk::to_string(binding.descriptorType),
                    this->available_descriptors[binding.descriptorType]);
            }
        }

        // the allocation will succeed decrement internal counts
        for (const auto& binding : layout->getLayoutBindings())
        {
            this->available_descriptors[binding.descriptorType] -=
                binding.descriptorCount;
        }

        const vk::DescriptorSetAllocateInfo descriptorAllocationInfo {

            .sType {vk::StructureType::eDescriptorSetAllocateInfo},
            .pNext {nullptr},
            .descriptorPool {*this->pool},
            .descriptorSetCount {1}, // TODO: add array function?
            .pSetLayouts {**layout},
        };

        // TODO: create an array function to deal with multiple allocation
        std::vector<vk::DescriptorSet> allocatedDescriptors =
            this->device->asLogicalDevice().allocateDescriptorSets(
                descriptorAllocationInfo);

        util::assertFatal(
            allocatedDescriptors.size() == 1,
            "Invalid descriptor length returned");

        return DescriptorSet {
            this->shared_from_this(),
            std::move(layout),
            allocatedDescriptors.at(0)};
    }

    DescriptorPool::DescriptorPool(
        std::shared_ptr<Device>                               device_,
        std::unordered_map<vk::DescriptorType, std::uint32_t> capacity)
        : device {std::move(device_)}
        , pool {nullptr} // , initial_descriptors {capacity}
        , available_descriptors {std::move(capacity)}
    {
        this->available_descriptors.rehash(32); // optimization

        std::vector<vk::DescriptorPoolSize> requestedPoolMembers {};

        for (auto& [descriptor, number] : this->available_descriptors)
        {
            requestedPoolMembers.push_back(vk::DescriptorPoolSize {
                .type {descriptor}, .descriptorCount {number}});
        }

        const vk::DescriptorPoolCreateInfo poolCreateInfo {
            .sType {vk::StructureType::eDescriptorPoolCreateInfo},
            .pNext {nullptr},
            .flags {vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
            .maxSets {64}, // why the **fuckk** is this needed
            // large number good enough :tm:
            .poolSizeCount {
                static_cast<std::uint32_t>(requestedPoolMembers.size())},
            .pPoolSizes {requestedPoolMembers.data()},
        };

        this->pool = this->device->asLogicalDevice().createDescriptorPoolUnique(
            poolCreateInfo);
    }

    void DescriptorPool::free(DescriptorSet& setToFree)
    {
        for (const auto& binding : setToFree.layout->getLayoutBindings())
        {
            this->available_descriptors[binding.descriptorType] +=
                binding.descriptorCount;
        }

        // TODO: add array function
        this->device->asLogicalDevice().freeDescriptorSets(
            *this->pool, *setToFree);
    }

    DescriptorSetLayout::DescriptorSetLayout(
        std::shared_ptr<Device>           device_,
        vk::DescriptorSetLayoutCreateInfo layoutCreateInfo)
        : device {std::move(device_)}
        , layout {nullptr}
        , descriptors {}
    {
        this->descriptors.reserve(layoutCreateInfo.bindingCount);

        for (std::size_t i = 0; i < layoutCreateInfo.bindingCount; ++i)
        {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
            this->descriptors.push_back(layoutCreateInfo.pBindings[i]);
#pragma clang diagnostic pop
        }

        this->layout =
            this->device->asLogicalDevice().createDescriptorSetLayoutUnique(
                layoutCreateInfo);
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
        , layout {nullptr}
        , pool {}
    {}

    DescriptorSet::~DescriptorSet()
    {
        if (static_cast<bool>(this->set))
        {
            this->pool->free(*this);
        }
    }

    DescriptorSet::DescriptorSet(DescriptorSet&& other)
        : set {other.set}
        , layout {std::move(other.layout)}
        , pool {std::move(other.pool)}
    {
        other.pool   = nullptr;
        other.set    = nullptr;
        other.layout = nullptr;
    }

    DescriptorSet& DescriptorSet::operator= (DescriptorSet&& other)
    {
        if (&other == this)
        {
            return *this;
        }

        this->pool   = std::move(other.pool);
        this->layout = std::move(other.layout);
        this->set    = std::move(other.set);

        other.pool   = nullptr;
        other.set    = nullptr;
        other.layout = nullptr;

        return *this;
    }

    vk::DescriptorSet DescriptorSet::operator* () const
    {
        return this->set;
    }

    DescriptorSet::DescriptorSet(
        std::shared_ptr<DescriptorPool>      pool_,
        std::shared_ptr<DescriptorSetLayout> layout_,
        vk::DescriptorSet                    set_)
        : set {set_}
        , layout {std::move(layout_)}
        , pool {std::move(pool_)}
    {}
} // namespace gfx::vulkan
