#pragma once

// 
#include <glm/glm.hpp>
#include <vkf/swapchain.hpp>

// 
#include "./core.hpp"

// 
namespace icv {

    // 
    struct BindingInfo {
        uint32_t buffer = 0u;
        uint32_t offset = 0u;
        uint32_t stride = 16u;
        uint32_t format = 0u; // reserved
    };

    // 
    struct GeometryRegistryInfo {
        std::vector<BindingInfo> bindings = {};
        std::vector<vkt::Vector> buffers = {};

        uint32_t maxBindingCount = 128u;
        
    };

    // 
    class GeometryRegistry: public DeviceBased {
        protected: 
        
        GeometryRegistryInfo info = {};

        // 
        vkt::Vector<BindingInfo> bindings = {};
        VkDescriptorSet set = VK_NULL_HANDLE;
        bool created = false;

        // 
        void constructor(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<GeometryRegistryInfo> info = GeometryRegistryInfo{}) {
            this->info = info;
            this->device = device;

            auto createBuffer = [&](FLAGS(VkBufferUsage) usage, VkDeviceSize size = 16ull, VkDeviceSize stride = sizeof(uint8_t)) {
                auto bufferCreateInfo = vkh::VkBufferCreateInfo{
                    .size = size,
                    .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | usage
                };
                auto vmaCreateInfo = vkt::VmaMemoryInfo{
                    .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                    .instanceDispatch = device->instance->dispatch,
                    .deviceDispatch = device->dispatch
                };
                auto allocation = std::make_shared<vkt::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
                return vkt::VectorBase(allocation, 0ull, size, sizeof(uint8_t));
            };

            this->bindings = createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(BindingInfo) * info->maxBindingCount, sizeof(BindingInfo));
        };
        public:
        GeometryRegistry() {};
        GeometryRegistry(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<GeometryRegistryInfo> info = GeometryRegistryInfo{}) { this->constructor(device, info); };

        // 
        void makeDescriptorSet(vkt::uni_arg<DescriptorInfo> info = DescriptorInfo{}) {
            vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(info->layout, device->descriptorPool);
            auto handle = descriptorSetHelper.pushDescription<uint64_t>(vkh::VkDescriptorUpdateTemplateEntry{
                .dstBinding = 0u,
                .descriptorCount = info.buffers.size(),
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            });
            for (uint32_t i=0;i<info.buffers.size();i++) {
                handle[i] = info.buffers[i];
            };
            descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry{
                .dstBinding = 1u,
                .descriptorCount = 1u,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            }) = bindings;
            vkh::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, set, created);
        };

        // 
        void flush(vkt::uni_ptr<vkf::Queue> queue = {}) {
            queue->uploadIntoBuffer(bindings, info.bindings.data(), std::min(info.bindings.size()*sizeof(BindingInfo), bindings.range()));
        };
    };

};
