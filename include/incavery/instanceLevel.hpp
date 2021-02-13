#pragma once

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"
#include "./geometryLevel.hpp"

// 
namespace icv {

    //
    struct InstanceInfo
    {   // 
        glm::mat3x4 transform = glm::mat3x4(1.f);

        // system ray tracing parameters
        uint8_t mask = 0xFFu;
        uint8_t flags = 0u;
        glm::u8vec2 todo = glm::u8vec2(0u);
        uint32_t sbtOffsetId = 0u;

        // WTF?! It used by renderer!
        uint32_t geometrylevelId = 0u;

        // 
        uint32_t geometryLevelCount = 0u;

        // used by GLSL (directly access), buffer reference
        uint64_t geometryInfoReference = 0ull;

        // acceleration structure reference (bottom level)
        uint64_t accelerationReference = 0ull; 

        // 
        void acceptGeometryLevel(vkh::uni_ptr<GeometryLevel> geometryLevel) {
            this->accelerationReference = geometryLevel->getDeviceAddress();
            this->geometryInfoReference = geometryLevel->getBuffer().deviceAddress();
        }
    };

    // 
    struct InstanceLevelInfo 
    {
        std::vector<InstanceInfo> instances = {};

        uint32_t maxInstanceCount = 128u;
    };

    // 
    class InstanceLevel: public DeviceBased {
        protected: 

        // 
        InstanceLevelInfo info = {};
        BuildInfo buildInfo = {};

        // 
        vkh::uni_ptr<DataSet<InstanceInfo>> instances = {};
        vkh::uni_ptr<DataSet<vkh::VkAccelerationStructureInstanceKHR>> nativeInstances = {};

        //
        VkDescriptorSet set = VK_NULL_HANDLE;
        bool created = false;

        // 
        VkAccelerationStructureKHR acceleration = VK_NULL_HANDLE;
        vkf::VectorBase accStorage = {};
        vkf::VectorBase accScratch = {};

        //
        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<InstanceLevelInfo> info = InstanceLevelInfo{}) 
        {
            this->info = info;
            this->device = device;

            this->instances = std::make_shared<DataSet<InstanceInfo>>(device, DataSetInfo{
                .count = info->maxInstanceCount,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            });

            this->nativeInstances = std::make_shared<DataSet<vkh::VkAccelerationStructureInstanceKHR>>(device, DataSetInfo{
                .count = info->maxInstanceCount,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            });
        };

        public: 
        InstanceLevel() {};
        InstanceLevel(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<InstanceLevelInfo> info = InstanceLevelInfo{}) { this->constructor(device, info); };

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
        virtual const vkf::Vector<vkh::VkAccelerationStructureInstanceKHR>& getBuffer() const {
            return nativeInstances->getDeviceBuffer();
        };
        
        //
        virtual vkf::Vector<vkh::VkAccelerationStructureInstanceKHR>& getBuffer() {
            return nativeInstances->getDeviceBuffer();
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
                    .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                    .descriptorCount = 1u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1 });
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 2u,
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

            vkt::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, set, created);
            return set;
        };

        // TODO: copy buffer
        virtual void buildCommand(VkCommandBuffer commandBuffer) 
        {
            // set native instances directly
            for (intptr_t i = 0; i < info.instances.size();i++) {
                nativeInstances->getCpuCache().at(i) = vkh::VkAccelerationStructureInstanceKHR{};
                nativeInstances->getCpuCache().at(i).transform = info.instances[i].transform;
                nativeInstances->getCpuCache().at(i).accelerationStructureReference = info.instances[i].accelerationReference;
                nativeInstances->getCpuCache().at(i).instanceShaderBindingTableRecordOffset = info.instances[i].sbtOffsetId;
                nativeInstances->getCpuCache().at(i).mask = info.instances[i].mask;
                nativeInstances->getCpuCache().at(i).flags = info.instances[i].flags;
            };

            if (!acceleration) { this->makeAccelerationStructure(); };
            {   // TODO: indirect condition
                nativeInstances->cmdCopyFromCpu(commandBuffer);
                instances->copyFromVector(info.instances);
                instances->cmdCopyFromCpu(commandBuffer);
            };
            buildInfo.ranges.resize(1u);
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
                        .data = this->nativeInstances->getDeviceBuffer().deviceAddress()
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
                this->accStorage = createBuffer(BufferCreateInfo{.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, .size = sizes.accelerationStructureSize});
                this->accScratch = createBuffer(BufferCreateInfo{.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, .size = sizes.buildScratchSize});
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
        virtual uintptr_t changeInstance(uintptr_t instanceId, vkh::uni_arg<InstanceInfo> info = InstanceInfo{})
        {   // add instance into registry
            if (this->info.instances.size() <= instanceId) { this->info.instances.resize(instanceId + 1u); };
            this->info.instances[instanceId] = info;
            return instanceId;
        };

        //
        virtual uintptr_t pushInstance(vkh::uni_arg<InstanceInfo> info = InstanceInfo{})
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
