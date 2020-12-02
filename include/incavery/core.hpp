#pragma once

// 
#include <glm/glm.hpp>
#include <vkf/swapchain.hpp>

// 
namespace icv {

    //
    struct DescriptorInfo {
        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        
    };

    //
    class DeviceBased {
        protected: 
        vkt::uni_ptr<vkf::Device> device = {};

        // 
        virtual void createBuffer(FLAGS(VkBufferUsage) usage, VkDeviceSize size = 16ull, VkDeviceSize stride = sizeof(uint8_t)) {
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

        // 
        virtual VkIndexType getIndexType(uint32_t indexType = 0u) {
            if (indexType == 0u) { return VK_INDEX_TYPE_NONE_KHR; }; 
            if (indexType == 1u) { return VK_INDEX_TYPE_UINT32; };
            if (indexType == 2u) { return VK_INDEX_TYPE_UINT16; };
            if (indexType == 3u) { return VK_INDEX_TYPE_UINT8_EXT; };
            return VK_INDEX_TYPE_NONE_KHR;
        };
    };

};
