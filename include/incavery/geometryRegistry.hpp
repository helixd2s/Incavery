#pragma once

// 
#include "./core.hpp"

// 
namespace icv {

#pragma pack(push, 8)
    //
    struct RawData
    {
        //uint32_t bufferId = 0u;
        //uint32_t offset = 0u;
        VkDeviceAddress data = 0ull;
    };

    // 
    struct BindingInfo 
    {
        uint32_t format = 0u; // reserved
        uint32_t stride = 16u;
        RawData ptr;
    };
#pragma pack(pop)

    // 
    struct GeometryRegistryInfo 
    {
        std::vector<BindingInfo> bindings = {};
        std::vector<vkh::VkDescriptorBufferInfo> buffers = {};
        //std::vector<vkt::VectorBase> buffers = {};

        uint32_t maxBindingCount = 128u;
        
    };

    // 
    class GeometryRegistry: public DeviceBased {
        protected: 
        GeometryRegistryInfo info = {};

        // 
        vkh::uni_ptr<DataSet<BindingInfo>> bindings = {};
        VkDescriptorSet set = VK_NULL_HANDLE;
        bool created = false;

        // 
        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<GeometryRegistryInfo> info = GeometryRegistryInfo{}) 
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
        GeometryRegistry(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<GeometryRegistryInfo> info = GeometryRegistryInfo{}) { this->constructor(device, info); };

        //
        static VkDescriptorSetLayout& createDescriptorSetLayout(vkh::uni_ptr<vkf::Device> device, VkDescriptorSetLayout& descriptorSetLayout) {
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            if (!descriptorSetLayout) 
            {   // create descriptor set layout
                vkh::VsDescriptorSetLayoutCreateInfoHelper descriptorSetLayoutHelper(vkh::VkDescriptorSetLayoutCreateInfo{});
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 0u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 256u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1 });
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 1u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1 });
                vkt::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &descriptorSetLayout));
            };

            return descriptorSetLayout;
        };

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
        virtual const vkf::Vector<BindingInfo>& getBuffer() const {
            return bindings->getDeviceBuffer();
        };

        //
        virtual vkf::Vector<BindingInfo>& getBuffer() {
            return bindings->getDeviceBuffer();
        };

        // 
        virtual VkDescriptorSet& makeDescriptorSet(vkh::uni_arg<DescriptorInfo> info = DescriptorInfo{}) 
        {
            vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(info->layout, device->descriptorPool);
            if (this->info.buffers.size() > 0ull) {
                auto handle = descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry{
                    .dstBinding = 0u,
                    .descriptorCount = uint32_t(this->info.buffers.size()),
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                });
                memcpy(&handle, this->info.buffers.data(), this->info.buffers.size() * sizeof(vkh::VkDescriptorBufferInfo));
            };
            descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry{
                .dstBinding = 1u,
                .descriptorCount = 1u,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            }) = bindings->getDeviceBuffer();
            vkt::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, set, created);
            return set;
        };

        //
        virtual void copyCommand(VkCommandBuffer commandBuffer)
        {   // 
            bindings->copyFromVector(info.bindings);
            bindings->cmdCopyFromCpu(commandBuffer);
        };

        // 
        virtual void flush(vkh::uni_ptr<vkf::Queue> queue = {}) 
        {   // 
            queue->submitOnce([&,this](VkCommandBuffer commandBuffer){
                this->copyCommand(commandBuffer);
            });
        };

        //
        virtual uintptr_t pushBuffer(vkh::VkDescriptorBufferInfo buffer) 
        {   
            uintptr_t index = this->info.buffers.size();
            this->info.buffers.push_back(buffer);
            return index;
        };

        //
        virtual uintptr_t pushBinding(vkh::uni_arg<BindingInfo> binding) 
        {   
            uintptr_t index = this->info.bindings.size();
            this->info.bindings.push_back(binding);
            return index;
        };

        //
        virtual uintptr_t pushBufferWithBinding(vkh::VkDescriptorBufferInfo buffer, vkh::uni_arg<BindingInfo> binding) 
        {   
            uintptr_t bufferId = pushBuffer(buffer);
            if (!binding->ptr.data) { binding->ptr.data = bufferDeviceAddress(info.buffers[bufferId]); };
            pushBinding(binding);
            return bufferId;
        };

        /*
        // TODO: separate set buffer or binding
        virtual void setBufferWithBinding(uintptr_t index, vkh::VkDescriptorBufferInfo buffer, vkh::uni_arg<BindingInfo> binding)
        {
            //binding->buffer = index; // prefer same index

            if (this->info.buffers.size() <= (binding->buffer)) { this->info.buffers.resize((binding->buffer)+1u); };
            if (this->info.bindings.size() <= index) { this->info.bindings.resize(index+1u); };

            this->info.buffers[binding->buffer] = buffer;
            this->info.bindings[index] = binding;
        };*/
    };

};
