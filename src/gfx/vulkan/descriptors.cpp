#include "descriptors.hpp"
#include "util/log.hpp"
#include <engine/settings.hpp>

namespace gfx::vulkan
{

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

    DescriptorSet DescriptorPool::allocate(DescriptorSetType typeToAllocate)
    {
        DescriptorSetLayout& layout =
            this->lookupOrAddLayoutFromCache(typeToAllocate);

        // ensure we have enough descriptors available for the desired
        // descriptor set
        for (const auto& binding : layout.getLayoutBindings())
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
        for (const auto& binding : layout.getLayoutBindings())
        {
            this->available_descriptors[binding.descriptorType] -=
                binding.descriptorCount;
        }

        const vk::DescriptorSetAllocateInfo descriptorAllocationInfo {

            .sType {vk::StructureType::eDescriptorSetAllocateInfo},
            .pNext {nullptr},
            .descriptorPool {*this->pool},
            .descriptorSetCount {1}, // TODO: add array function?
            .pSetLayouts {*layout},
        };

        // TODO: create an array function to deal with multiple allocation
        std::vector<vk::DescriptorSet> allocatedDescriptors =
            this->device.allocateDescriptorSets(descriptorAllocationInfo);

        util::assertFatal(
            allocatedDescriptors.size() == 1,
            "Invalid descriptor length returned");

        return DescriptorSet {allocatedDescriptors.at(0), this, &layout};
    }

    DescriptorPool::DescriptorPool(
        vk::Device                                            device_,
        std::unordered_map<vk::DescriptorType, std::uint32_t> capacity)
        : device {device_}
        , pool {nullptr}
        , inital_descriptors {capacity}
        , available_descriptors {std::move(capacity)}
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
            .maxSets {64}, // why the **fuckk** is this needed
            // large number good enough :tm:
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

                for (const auto [initalDescriptorType, initalNum] :
                     this->inital_descriptors)
                {
                    util::logWarn(
                        "Inital #{}, final #{} of type {}",
                        initalNum,
                        this->available_descriptors[initalDescriptorType],
                        vk::to_string(initalDescriptorType));
                }
            }
        }
    }

    DescriptorSetLayout&
    DescriptorPool::lookupOrAddLayoutFromCache(DescriptorSetType typeToGet)
    {
        if (this->descriptor_layout_cache.contains(typeToGet))
        {
            return this->descriptor_layout_cache[typeToGet];
        }
        else
        {
            switch (typeToGet)
            {
            case DescriptorSetType::None:
                util::panic(
                    "Tried to find the layout of a DescriptorSetType::None!");
                break;
            case DescriptorSetType::Voxel: {
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

                this->descriptor_layout_cache[typeToGet] = DescriptorSetLayout {
                    this->device,
                    vk::DescriptorSetLayoutCreateInfo {
                        .sType {
                            vk::StructureType::eDescriptorSetLayoutCreateInfo},
                        .pNext {nullptr},
                        .flags {},
                        .bindingCount {
                            static_cast<std::uint32_t>(bindings.size())},
                        .pBindings {bindings.data()},
                    }};
                break;
            }
            case DescriptorSetType::VoxelRayTracing: {
                std::array<vk::DescriptorSetLayoutBinding, 3> bindings {
                    vk::DescriptorSetLayoutBinding {
                        .binding {0},
                        .descriptorType {vk::DescriptorType::eStorageImage},
                        .descriptorCount {1},
                        .stageFlags {vk::ShaderStageFlagBits::eCompute},
                        .pImmutableSamplers {nullptr},
                    },
                    vk::DescriptorSetLayoutBinding {
                        .binding {1},
                        .descriptorType {vk::DescriptorType::eUniformBuffer},
                        .descriptorCount {1},
                        .stageFlags {vk::ShaderStageFlagBits::eCompute},
                        .pImmutableSamplers {nullptr},
                    },
                    vk::DescriptorSetLayoutBinding {
                        .binding {2},
                        .descriptorType {vk::DescriptorType::eStorageBuffer},
                        .descriptorCount {1},
                        .stageFlags {vk::ShaderStageFlagBits::eCompute},
                        .pImmutableSamplers {nullptr},
                    },
                };

                this->descriptor_layout_cache[typeToGet] = DescriptorSetLayout {
                    this->device,
                    vk::DescriptorSetLayoutCreateInfo {
                        .sType {
                            vk::StructureType::eDescriptorSetLayoutCreateInfo},
                        .pNext {nullptr},
                        .flags {},
                        .bindingCount {
                            static_cast<std::uint32_t>(bindings.size())},
                        .pBindings {bindings.data()},
                    }};
            }
            }

            return this->descriptor_layout_cache[typeToGet];
        }
    }

    // TODO: add array function
    void DescriptorPool::free(DescriptorSet& setToFree)
    {
        for (const auto& binding : setToFree.layout->getLayoutBindings())
        {
            this->available_descriptors[binding.descriptorType] +=
                binding.descriptorCount;
        }

        // TODO: add array function
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
        , pool {nullptr}
        , layout {nullptr}
    {}

    DescriptorSet::~DescriptorSet()
    {
        if (this->set != nullptr)
        {
            this->pool->free(*this);
        }
    }

    DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
        : set {other.set}
        , pool {other.pool}
        , layout {other.layout}
    {
        other.set    = nullptr;
        other.pool   = nullptr;
        other.layout = nullptr;
    }

    DescriptorSet& DescriptorSet::operator= (DescriptorSet&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        this->set    = other.set;
        this->pool   = other.pool;
        this->layout = other.layout;

        other.set    = nullptr;
        other.pool   = nullptr;
        other.layout = nullptr;

        return *this;
    }

    vk::DescriptorSet DescriptorSet::operator* () const
    {
        return this->set;
    }

    DescriptorSet::DescriptorSet(
        vk::DescriptorSet    set_,
        DescriptorPool*      pool_,
        DescriptorSetLayout* layout_)
        : set {set_}
        , pool {pool_}
        , layout {layout_}
    {}
} // namespace gfx::vulkan
