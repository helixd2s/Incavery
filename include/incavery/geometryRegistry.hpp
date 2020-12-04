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
                .count = info->maxBindingCount,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            });
        };
        
        public:
        GeometryRegistry() {};
        GeometryRegistry(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<GeometryRegistryInfo> info = GeometryRegistryInfo{}) { this->constructor(device, info); };

        //
        virtual const VkDescriptorSet& getDescriptorSet() const {
            return set;
        };

        //
        virtual VkDescriptorSet& getDescriptorSet() {
            return set;
        };

        //
        virtual const GeometryRegistryInfo& getInfo() const {
            return info;
        };

        //
        virtual GeometryRegistryInfo& getInfo() {
            return info;
        };

        //
        virtual const vkt::Vector<BindingInfo>& getBuffer() const {
            return bindings->getDeviceBuffer();
        };

        //
        virtual vkt::Vector<BindingInfo>& getBuffer() {
            return bindings->getDeviceBuffer();
        };

        // 
        virtual VkDescriptorSet& makeDescriptorSet(vkt::uni_arg<DescriptorInfo> info = DescriptorInfo{}) 
        {
            vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(info->layout, device->descriptorPool);
            auto handle = descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry{
                .dstBinding = 0u,
                .descriptorCount = uint32_t(this->info.buffers.size()),
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            });
            for (uint32_t i=0;i<this->info.buffers.size();i++) 
            {
                handle[i] = this->info.buffers[i];
            };
            descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry{
                .dstBinding = 1u,
                .descriptorCount = 1u,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            }) = bindings->getDeviceBuffer();
            vkh::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, set, created);
            return set;
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
        virtual uintptr_t pushBuffer(vkt::VectorBase buffer) 
        {   
            uintptr_t index = this->info.buffers.size();
            this->info.buffers.push_back(buffer);
            return index;
        };

        //
        virtual uintptr_t pushBinding(vkt::uni_arg<BindingInfo> binding) 
        {   
            uintptr_t index = this->info.bindings.size();
            this->info.bindings.push_back(binding);
            return index;
        };

        //
        virtual uintptr_t pushBufferWithBinding(vkt::VectorBase buffer, vkt::uni_arg<BindingInfo> binding) 
        {   
            binding->buffer = pushBuffer(buffer);
            pushBinding(binding);
            return binding->buffer;
        };

        // 
        virtual void setBufferWithBinding(uintptr_t index, vkt::VectorBase buffer, vkt::uni_arg<BindingInfo> binding)
        {
            //binding->buffer = index; // prefer same index

            if (this->info.buffers.size() <= (binding->buffer)) { this->info.buffers.resize((binding->buffer)+1u); };
            if (this->info.bindings.size() <= index) { this->info.bindings.resize(index+1u); };

            this->info.buffers[binding->buffer] = buffer;
            this->info.bindings[index] = binding;
        };
    };

};
