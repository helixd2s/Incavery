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

    // Vulkan needs f&cking SoA, EVERY TIME!
    struct BuildInfo 
    {
        std::vector<vkh::VkAccelerationStructureGeometryKHR> builds = {};
        std::vector<vkh::VkAccelerationStructureBuildRangeInfoKHR> ranges = {};
        vkh::VkAccelerationStructureBuildGeometryInfoKHR info = {};
    };

    //
    class DeviceBased {
        protected: 
        vkt::uni_ptr<vkf::Device> device = {};
        vkh::VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {};
        
        // 
        virtual vkt::VectorBase createBuffer(FLAGS(VkBufferUsage) usage, VkDeviceSize size = 16ull, VkDeviceSize stride = sizeof(uint8_t)) 
        {   // 
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
        virtual vkt::ImageRegion createImage2D(FLAGS(VkImageUsage) usage, VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT, vkh::VkExtent3D size = {}, bool isDepth = false)
        {   // 
            vkh::VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = format;
            imageCreateInfo.extent = vkh::VkExtent3D{ size.x, size.y, 1u };
            imageCreateInfo.mipLevels = 1u;
            imageCreateInfo.arrayLayers = 1u;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            // 
            auto vmaCreateInfo = vkt::VmaMemoryInfo{
                .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                .instanceDispatch = instance->dispatch,
                .deviceDispatch = device->dispatch
            };

            // 
            vkh::VkImageViewCreateInfo imageViewCreateInfo = {};
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = format;
            imageViewCreateInfo.components = vkh::VkComponentMapping{};
            imageViewCreateInfo.subresourceRange = vkh::VkImageSubresourceRange{ .aspectMask = isDepth?(VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT):(VK_IMAGE_ASPECT_COLOR_BIT), .baseMipLevel = 0u, .levelCount = 1u, .baseArrayLayer = 0u, .layerCount = 1u };

            // 
            auto allocation = std::make_shared<vkt::VmaImageAllocation>(device->allocator, imageCreateInfo, vmaCreateInfo);
            return vkt::ImageRegion(allocation, imageViewCreateInfo, isDepth?VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:VK_IMAGE_LAYOUT_GENERAL);
        };

        // 
        virtual VkIndexType getIndexType(uint32_t indexType = 0u) 
        {   // 
            if (indexType == 0u) { return VK_INDEX_TYPE_NONE_KHR; }; 
            if (indexType == 1u) { return VK_INDEX_TYPE_UINT32; };
            if (indexType == 2u) { return VK_INDEX_TYPE_UINT16; };
            if (indexType == 3u) { return VK_INDEX_TYPE_UINT8_EXT; };
            return VK_INDEX_TYPE_NONE_KHR;
        };
    };

};
