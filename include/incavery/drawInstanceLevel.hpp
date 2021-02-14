#pragma once

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"
#include "./geometryLevel.hpp"


// 
namespace icv {

    // 
    struct DrawInstance {
        glm::mat3x4 transform = glm::mat3x4(1.f);

        // 
        uint32_t programId = 0u;

        // WTF?! It used by renderer!
        uint32_t geometryLevelId = 0u;

        //
        uint32_t geometryLevelCount = 0u;

        // 
        uint32_t reserved = 0u;

        // used by GLSL (directly access)
        uint64_t geometryLevelReference = 0ull;

        // for indirect acceleration structure GPU supported
        uint64_t geometryLevelIndirectReference = 0ull;

        // 
        void acceptGeometryLevel(vkh::uni_ptr<GeometryLevel> geometryLevel) {
            this->geometryLevelIndirectReference = geometryLevel->getIndirectBuildBuffer().deviceAddress();
            this->geometryLevelReference = geometryLevel->getBuffer().deviceAddress();
        }
    };

    // 
    struct DrawInstanceLevelInfo 
    {
        // reserved for future usage
        //vkh::uni_ptr<InstanceLevel> instanceLevel = {};

        // now, used by renderer
        //std::vector<GeometryLevel> geometries = {};

        // 
        std::vector<DrawInstance> instances = {};
        uint32_t maxInstanceCount = 128u;
    };

    // 
    class DrawInstanceLevel: public DeviceBased {
        protected: 

        // 
        DrawInstanceLevelInfo info = {};

        // 
        std::vector<vkf::Vector<VkDrawIndirectCommand>> indirectDrawBuffers = {};
        vkh::uni_ptr<DataSet<DrawInstance>> instances = {};

        //
        VkDescriptorSet set = VK_NULL_HANDLE;
        bool created = false;

        //
        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<DrawInstanceLevelInfo> info = DrawInstanceLevelInfo{}) 
        {
            this->info = info;
            this->device = device;
            this->instances = std::make_shared<DataSet<DrawInstance>>(device, DataSetInfo{
                .count = info->maxInstanceCount,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            });
        };

        public: 
        DrawInstanceLevel() {};
        DrawInstanceLevel(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<DrawInstanceLevelInfo> info = DrawInstanceLevelInfo{}) { this->constructor(device, info); };

        //
        virtual const VkDescriptorSet& getDescriptorSet() const {
            return set;
        };

        //
        virtual VkDescriptorSet& getDescriptorSet() {
            return set;
        };

        //
        virtual const DrawInstanceLevelInfo& getInfo() const {
            return info;
        };

        //
        virtual DrawInstanceLevelInfo& getInfo() {
            return info;
        };

        //
        virtual vkf::Vector<VkDrawIndirectCommand>& getIndirectDrawBuffer(const intptr_t& I = 0u) {
            return indirectDrawBuffers[I];
        };

        //
        static VkDescriptorSetLayout& createDescriptorSetLayout(vkh::uni_ptr<vkf::Device> device, VkDescriptorSetLayout& descriptorSetLayout) {
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            {   // create descriptor set layout
                vkh::VsDescriptorSetLayoutCreateInfoHelper descriptorSetLayoutHelper(vkh::VkDescriptorSetLayoutCreateInfo{});
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 0u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1 });
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 1u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 256u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1 });
                vkt::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &descriptorSetLayout));
            };
            return descriptorSetLayout;
        };

        //
        virtual VkDescriptorSet& makeDescriptorSet(vkh::uni_arg<DescriptorInfo> info = DescriptorInfo{}) 
        {
            // create descriptor set
            vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(info->layout, device->descriptorPool);
            descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry
            {
                .dstBinding = 0u,
                .descriptorCount = 1u,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            }) = instances->getDeviceBuffer();

            // for indirect filling (needs compute shader)
            if (indirectDrawBuffers.size() > 0ull) {
                auto handle = descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry{
                    .dstBinding = 1u,
                    .descriptorCount = uint32_t(indirectDrawBuffers.size()),
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                });
                for (intptr_t i=0;i<indirectDrawBuffers.size();i++) {
                    handle[i] = indirectDrawBuffers[i];
                };
            };

            vkt::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, set, created);
            return set;
        };


        //
        virtual void setGeometryReferences(const std::vector<vkh::uni_ptr<GeometryLevel>>& geometries) {
            // reload geometries from list to reference
            for (intptr_t i=0;i<info.instances.size();i++) {
                info.instances[i].acceptGeometryLevel(geometries[info.instances[i].geometryLevelId]);
            };
        };

        // 
        virtual void buildCommand(VkCommandBuffer commandBuffer) 
        {
            
            
            {   // 
                instances->copyFromVector(info.instances);
                instances->cmdCopyFromCpu(commandBuffer);
            };
        };



        



        //
        virtual uintptr_t changeInstance(uintptr_t instanceId, vkh::uni_arg<DrawInstance> info = DrawInstance{})
        {   // add instance into registry
            if (this->info.instances.size() <= instanceId) { this->info.instances.resize(instanceId + 1u); };
            this->info.instances[instanceId] = info;
            return instanceId;
        };

        //
        virtual uintptr_t pushInstance(vkh::uni_arg<DrawInstance> info = DrawInstance{})
        {   // add instance into registry
            uintptr_t instanceId = this->info.instances.size();
            this->info.instances.push_back(info);
            return instanceId;
        };



        // 
        virtual void flush(vkh::uni_ptr<vkf::Queue> queue = {}) 
        {   // 
            queue->submitOnce([&,this](VkCommandBuffer commandBuffer) 
            {   // 
                this->buildCommand(commandBuffer);
            });
        };
    };

};
