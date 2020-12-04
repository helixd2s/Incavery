#pragma once

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"
#include "./geometryLevel.hpp"

// 
namespace icv {

    //
    using InstanceInfo = VkAccelerationStructureInstanceKHR;

    // 
    struct InstanceLevelInfo 
    {
        std::vector<vkt::uni_ptr<GeometryLevel>> geometries = {}; // ONLY for descriptor sets
        std::vector<InstanceInfo> instances = {};
        
        uint32_t maxInstanceCount = 128u;
    };

    // 
    class InstanceLevel: public DeviceBased {
        protected: 
        vkt::uni_ptr<GeometryRegistry> registry = {};

        // 
        InstanceLevelInfo info = {};
        BuildInfo buildInfo = {};

        // 
        vkt::uni_ptr<DataSet<InstanceInfo>> instances = {};
        VkDescriptorSet set = VK_NULL_HANDLE;
        bool created = false;

        // 
        VkAccelerationStructureKHR acceleration = VK_NULL_HANDLE;
        vkt::VectorBase accStorage = {};
        vkt::VectorBase accScratch = {};

        //
        virtual void constructor(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<InstanceLevelInfo> info = InstanceLevelInfo{}) 
        {
            this->info = info;
            this->device = device;
            this->instances = std::make_shared<DataSet<InstanceInfo>>(device, DataSetInfo{
                .count = info->maxInstanceCount,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            });
        };

        public: 
        InstanceLevel() {};
        InstanceLevel(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<InstanceLevelInfo> info = InstanceLevelInfo{}) { this->constructor(device, info); };

        //
        virtual const VkDescriptorSet& getDescriptorSet() const {
            return set;
        };

        //
        virtual VkDescriptorSet& getDescriptorSet() {
            return set;
        };

        //
        virtual const InstanceLevelInfo& getInfo() const {
            return info;
        };

        //
        virtual InstanceLevelInfo& getInfo() {
            return info;
        };

        //
        virtual const vkt::Vector<InstanceInfo>& getBuffer() const {
            return instances->getDeviceBuffer();
        };

        //
        virtual vkt::Vector<InstanceInfo>& getBuffer() {
            return instances->getDeviceBuffer();
        };

        //
        virtual VkDeviceAddress getDeviceAddress() {
            if (!acceleration) { this->makeAccelerationStructure(); };
            return device->dispatch->GetAccelerationStructureDeviceAddressKHR(&(deviceAddressInfo = acceleration));
        };

        //
        //virtual VkDeviceAddress getDeviceAddress() const {
        //    vkh::VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {};
        //    return device->dispatch->GetAccelerationStructureDeviceAddressKHR(&(deviceAddressInfo = acceleration));
        //};

        //
        virtual VkDescriptorSet& makeDescriptorSet(vkt::uni_arg<DescriptorInfo> info = DescriptorInfo{}) 
        {
            if (!acceleration) {
                this->makeAccelerationStructure();
            };

            // create descriptor set
            vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(info->layout, device->descriptorPool);
            descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry
            {
                .dstBinding = 0u,
                .descriptorCount = 1u,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            }) = instances->getDeviceBuffer();
            descriptorSetHelper.pushDescription<VkAccelerationStructureKHR>(vkh::VkDescriptorUpdateTemplateEntry
            {
                .dstBinding = 1u,
                .descriptorCount = 1u,
                .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
            }) = acceleration;

            // geometries needed for handle instance intersection
            auto handle = descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry
            {
                .dstBinding = 2u,
                .descriptorCount = uint32_t(this->info.geometries.size()),
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            });
            for (uint32_t i=0;i<this->info.geometries.size();i++) {
                handle[i] = this->info.geometries[i]->getBuffer();
            };

            vkh::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, set, created);
            return set;
        };

        // TODO: copy buffer
        virtual void buildCommand(VkCommandBuffer commandBuffer) 
        {
            //for (uint32_t i=0;i<this->info.geometries.size();i++) {
            //    this->info.geometries[i]->buildCommand(commandBuffer);
            //};
            {   // TODO: indirect condition
                instances->copyFromVector(info.instances);
                instances->cmdCopyFromCpu(commandBuffer);
            };
            buildInfo.ranges[0u].primitiveCount = info.instances.size();
            
            const auto ptr = &buildInfo.ranges[0u];
            device->dispatch->CmdBuildAccelerationStructuresKHR(commandBuffer, 1u, &buildInfo.info, &ptr);
        };

        // 
        virtual void makeAccelerationStructure() 
        {
            auto accelerationStructureType = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

            {   // 
                buildInfo.builds.resize(1u);
                buildInfo.ranges.resize(1u); 
                {
                    // fill ranges
                    buildInfo.ranges[0u].primitiveCount = info.instances.size();

                    // fill build info
                    buildInfo.builds[0u].geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
                    buildInfo.builds[0u].geometry = vkh::VkAccelerationStructureGeometryInstancesDataKHR
                    {
                        .arrayOfPointers = false,
                        .data = this->instances->getDeviceBuffer().deviceAddress()
                    };
                };
                buildInfo.info.type = accelerationStructureType;
                buildInfo.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
                buildInfo.info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
                buildInfo.info.geometryCount = buildInfo.builds.size();
                buildInfo.info.pGeometries = &buildInfo.builds[0];
            };

            vkh::VkAccelerationStructureBuildSizesInfoKHR sizes = {};
            {   // 
                std::vector<uint32_t> primitiveCount = { uint32_t(info.instances.size()) };
                device->dispatch->GetAccelerationStructureBuildSizesKHR(VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo.info, primitiveCount.data(), &sizes);
            };

            {   // 
                this->accStorage = createBuffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, sizes.accelerationStructureSize);
                this->accScratch = createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, sizes.buildScratchSize);
            };

            {   // create acceleration structure
                vkh::VkAccelerationStructureCreateInfoKHR accelerationInfo = {};
                accelerationInfo.type = accelerationStructureType;
                accelerationInfo = this->accStorage;
                device->dispatch->CreateAccelerationStructureKHR(accelerationInfo, nullptr, &this->acceleration);
            };

            {   //
                buildInfo.info.dstAccelerationStructure = this->acceleration;
                buildInfo.info.scratchData = this->accScratch;
            };
        };

        //
        virtual uintptr_t changeInstance(uintptr_t instanceId, vkt::uni_arg<InstanceInfo> info = InstanceInfo{})
        {   // add instance into registry
            info->accelerationStructureReference = this->info.geometries[info->instanceCustomIndex]->getDeviceAddress();
            if (this->info.instances.size() <= instanceId) { this->info.instances.resize(instanceId+1u); };
            this->info.instances[instanceId] = info;
            return instanceId;
        };

        //
        virtual uintptr_t changeGeometryLevel(uintptr_t geometryId, vkt::uni_ptr<GeometryLevel> geometry = {})
        {   // add geometry to registry
            if (this->info.geometries.size() <= geometryId) { this->info.geometries.resize(geometryId+1u); };
            this->info.geometries[geometryId] = geometry;
            return geometryId;
        };

        //
        virtual uintptr_t pushInstance(vkt::uni_arg<InstanceInfo> info = InstanceInfo{})
        {   // add instance into registry
            uintptr_t instanceId = this->info.instances.size();
            info->accelerationStructureReference = this->info.geometries[info->instanceCustomIndex]->getDeviceAddress();
            this->info.instances.push_back(info);
            return instanceId;
        };

        //
        virtual uintptr_t pushGeometryLevel(vkt::uni_ptr<GeometryLevel> geometry = {})
        {   // add geometry to registry
            uintptr_t geometryId = this->info.geometries.size();
            this->info.geometries.push_back(geometry);
            return geometryId;
        };

        // 
        virtual void flush(vkt::uni_ptr<vkf::Queue> queue = {}) 
        {   // 
            queue->submitOnce([&,this](VkCommandBuffer commandBuffer) 
            {   // 
                this->buildCommand(commandBuffer);
            });
        };
    };

};
