#pragma once

// 
#include <glm/glm.hpp>
#include <vkf/swapchain.hpp>

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"

// 
namespace icv {

    // 
    struct Attributes {
        uint32_t texcoords = 0u;
        uint32_t normals = 0u;
        uint32_t tangents = 0u;
        uint32_t colors = 0u;
    };

    // 
    struct VertexInfo {
        uint32_t buffer = 0u;
        uint32_t offset = 0u;
        uint32_t stride = 16u;
        uint32_t max = 0u;
    };

    //
    struct IndexInfo {
        uint32_t buffer = 0u;
        uint32_t type = 0u; // 0 = none, 1 = uint32_t, 2 = uint16_t, 3 = uint8_t
    };

    // 
    struct GeometryInfo {
        glm::mat3x4 transform = glm::mat3x4(1.f);
        uint32_t 
            isOpaque: 1,
            useHalf : 1,
            hasTexcoords: 1, 
            hasNormals: 1,
            hasTangents: 1,
            hasColors: 1,
            hasTransform: 1;
            
        uint32_t primitiveCount = 1u;
        
        VertexInfo vertex = {};
        IndexInfo index = {};
        Attributes attributes = {};
    };

    // 
    struct GeometryLevelInfo {
        std::vector<GeometryInfo> geometries = {};
        
        uint32_t maxGeometryCount = 128u;
    };

    // Vulkan needs f&cking SoA, EVERY TIME!
    //struct GeometryBuildInfo {
    //    vkh::VkAccelerationStructureGeometryKHR buildInfo = {};
    //    vkh::VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
    //};

    // 
    struct BuildInfo {
        std::vector<VkAccelerationStructureGeometryKHR*> builds = {};
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> ranges = {};
        vkh::VkAccelerationStructureBuildGeometryInfoKHR info = {};
    };

    // 
    class GeometryLevel: public DeviceBased {
        protected: 
        vkt::uni_ptr<vkf::Device> device = {};
        GeometryLevelInfo info = {};
        BuildInfo buildInfo = {};

        // 
        vkt::Vector<GeometryInfo> geometries = {};
        VkDescriptorSet set = VK_NULL_HANDLE;
        bool created = false;

        // 
        VkAccelerationStructureKHR acceleration = VK_NULL_HANDLE;
        

        //
        void constructor(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<GeometryLevelInfo> info = GeometryLevelInfo{}) {
            this->info = info;
            this->device = device;

            this->geometries = createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(GeometryInfo) * info->maxGeometryCount, sizeof(GeometryInfo));
        };

        public: 
        GeometryLevel() {};
        GeometryLevel(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<GeometryLevelInfo> info = GeometryLevelInfo{}) { this->constructor(device, info); };

        // 
        void makeDescriptorSet(vkt::uni_arg<DescriptorInfo> info = DescriptorInfo{}) {
            // create descriptor set
            vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(info->layout, device->descriptorPool);
            descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry{
                .dstBinding = 0u,
                .descriptorCount = 1u,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            }) = geometries;
            vkh::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, set, created);
        };

        // TODO: copy buffer
        void buildCommand(VkCommandBuffer commandBuffer) {
            
            device->CmdBuildAccelerationStructuresKHR(commandBuffer, 1u, buildInfo.info, buildInfo.ranges.data());
        };

        //
        void makeAccelerationStructure() {
            
        };

        // 
        void flush(vkt::uni_ptr<vkf::Queue> queue = {}) {
            queue->uploadIntoBuffer(geometries, info.geometries.data(), std::min(info.geometries.size()*sizeof(GeometryInfo), geometries.range()));
            
        };
    };

};
