#pragma once

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
        vkt::uni_ptr<DataSet<BindingInfo>> bindings = {};
        VkDescriptorSet set = VK_NULL_HANDLE;
        bool created = false;

        // 
        virtual void constructor(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<GeometryRegistryInfo> info = GeometryRegistryInfo{}) 
        {
            this->info = info;
            this->device = device;
            this->bindings = std::make_shared<DataSet<BindingInfo>>(device, DataSetInfo{
                .count = info->maxBindingCount
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            });
        };
        
        public:
        GeometryRegistry() {};
        GeometryRegistry(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<GeometryRegistryInfo> info = GeometryRegistryInfo{}) { this->constructor(device, info); };

        //
        virtual const vkt::Vector<BindingInfo>& getBuffer() const {
            return bindings.getDeviceBuffer();
        };

        //
        virtual vkt::Vector<BindingInfo>& getBuffer() {
            return bindings.getDeviceBuffer();
        };

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
        virtual void copyCommand(VkCommandBuffer commandBuffer)
        {   // 
            bindings->copyFromVector(info.bindings);
            bindings->cmdCopyFromCpu(commandBuffer);
        };

        // 
        virtual void flush(vkt::uni_ptr<vkf::Queue> queue = {}) 
        {   // 
            queue->submitOnce([&,this](VkCommandBuffer commandBuffer){
                this->copyCommand(commandBuffer);
            });
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
            binding->buffer = index; // prefer same index

            if (this->info.buffers.size() <= (binding->buffer)) { this->info.buffers.resize((binding->buffer)+1u); };
            if (this->info.bindings.size() <= index) { this->info.bindings.resize(index+1u); };

            this->info.buffers[binding->buffer] = buffer;
            this->info.bindings[index] = binding;
        };
    };

};
