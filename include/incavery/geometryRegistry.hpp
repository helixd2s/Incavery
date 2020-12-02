#pragma once

// 
#include <glm/glm.hpp>
#include <vkf/swapchain.hpp>

// 
#include "./core.hpp"

// 
namespace icv {

    // 
    struct BindingInfo 
    {
        uint32_t format = 0u; // reserved
        uint32_t buffer = 0u;
        uint32_t offset = 0u;
        uint32_t stride = 16u;
    };

    // 
    struct GeometryRegistryInfo 
    {
        std::vector<BindingInfo> bindings = {};
        std::vector<vkt::VectorBase> buffers = {};

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
        virtual void constructor(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<GeometryRegistryInfo> info = GeometryRegistryInfo{}) 
        {
            this->info = info;
            this->device = device;
            this->bindings = createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(BindingInfo) * info->maxBindingCount, sizeof(BindingInfo));
        };
        
        public:
        GeometryRegistry() {};
        GeometryRegistry(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<GeometryRegistryInfo> info = GeometryRegistryInfo{}) { this->constructor(device, info); };

        // 
        virtual void makeDescriptorSet(vkt::uni_arg<DescriptorInfo> info = DescriptorInfo{}) 
        {
            vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(info->layout, device->descriptorPool);
            auto handle = descriptorSetHelper.pushDescription<uint64_t>(vkh::VkDescriptorUpdateTemplateEntry{
                .dstBinding = 0u,
                .descriptorCount = info.buffers.size(),
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            });
            for (uint32_t i=0;i<info.buffers.size();i++) 
            {
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
        virtual void flush(vkt::uni_ptr<vkf::Queue> queue = {}) 
        {
            queue->uploadIntoBuffer(bindings, info.bindings.data(), std::min(info.bindings.size()*sizeof(BindingInfo), bindings.range()));
        };

        //
        virtual void pushBinding(vkt::VectorBase buffer, vkt::uni_arg<BindingInfo> binding) 
        {   
            binding->buffer = this->info.buffers.size();
            this->info.buffers.push_back(buffer);
            this->info.bindings.push_back(binding);
        };

        // 
        virtual void setBinding(uintptr_t index, vkt::VectorBase buffer, vkt::uni_arg<BindingInfo> binding)
        {
            if (this->info.buffers.size() <= index) { this->info.buffers.resize(index+1u); };
            if (this->info.bindings.size() <= index) { this->info.bindings.resize(index+1u); };
            binding->buffer = index;
            this->info.buffers[index] = buffer;
            this->info.bindings[index] = binding;
        };
    };

};
