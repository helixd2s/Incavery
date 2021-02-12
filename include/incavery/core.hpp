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
    struct BufferCreateInfo
    {
        FLAGS(VkBufferUsage) usage;
        VkDeviceSize size = 16ull;
        VkDeviceSize stride = sizeof(uint8_t);
        VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    };

    //
    struct ImageCreateInfo
    {
        FLAGS(VkImageUsage) usage;
        VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vkh::VkExtent3D extent = {};
        bool isDepth = false;
    };

    // 
    enum class IndexType : uint32_t {
        None = 0u,
        Uint32 = 1u,
        Uint16 = 2u,
        Uint8 = 3u
    };


    //
    class DeviceBased {
        protected: 
        vkh::uni_ptr<vkf::Device> device = {};
        vkh::VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo = {};
        vkh::VkBufferDeviceAddressInfo bufferAddressInfo = {};
        
        //
        virtual VkDeviceAddress bufferDeviceAddress(vkh::VkDescriptorBufferInfo buffer) 
        {
            return (device->dispatch->GetBufferDeviceAddress(bufferAddressInfo = buffer.buffer) + buffer.offset);
        };

        // 
        virtual vkf::VectorBase createBuffer(vkh::uni_arg<BufferCreateInfo> info) 
        {   // 
            auto bufferCreateInfo = vkh::VkBufferCreateInfo{
                .size = info->size,
                .usage = (info->memoryUsage == VMA_MEMORY_USAGE_GPU_ONLY ? VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT : 0u) | VkBufferUsageFlags(info->usage)
            };
            auto vmaCreateInfo = vkf::VmaMemoryInfo{
                .memUsage = info->memoryUsage,
                .instanceDispatch = device->instance->dispatch,
                .deviceDispatch = device->dispatch
            };
            auto allocation = std::make_shared<vkf::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
            return vkf::VectorBase(allocation, 0ull, info->size, sizeof(uint8_t));
        };

        //
        virtual vkf::ImageRegion createImage2D(vkh::uni_arg<ImageCreateInfo> info)
        {   // 
            vkh::VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = info->format;
            imageCreateInfo.extent = vkh::VkExtent3D{ info->extent.width, info->extent.height, 1u };
            imageCreateInfo.mipLevels = 1u;
            imageCreateInfo.arrayLayers = 1u;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.usage = info->usage | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            // 
            auto vmaCreateInfo = vkf::VmaMemoryInfo{
                .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                .instanceDispatch = device->instance->dispatch,
                .deviceDispatch = device->dispatch
            };

            //
            auto aspectFlags = info->isDepth ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : (VK_IMAGE_ASPECT_COLOR_BIT);

            // 
            vkh::VkImageViewCreateInfo imageViewCreateInfo = {};
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = info->format;
            imageViewCreateInfo.components = vkh::VkComponentMapping{};
            imageViewCreateInfo.subresourceRange = vkh::VkImageSubresourceRange{ .aspectMask = VkImageAspectFlags(aspectFlags), .baseMipLevel = 0u, .levelCount = 1u, .baseArrayLayer = 0u, .layerCount = 1u };

            // 
            auto allocation = std::make_shared<vkf::VmaImageAllocation>(device->allocator, imageCreateInfo, vmaCreateInfo);
            return vkf::ImageRegion(allocation, imageViewCreateInfo, info->isDepth?VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:VK_IMAGE_LAYOUT_GENERAL);
        };

        // 
        virtual VkIndexType getIndexType(IndexType indexType = IndexType::None)
        {   // 
            if (indexType == IndexType::None) { return VK_INDEX_TYPE_NONE_KHR; };
            if (indexType == IndexType::Uint32) { return VK_INDEX_TYPE_UINT32; };
            if (indexType == IndexType::Uint16) { return VK_INDEX_TYPE_UINT16; };
            if (indexType == IndexType::Uint8) { return VK_INDEX_TYPE_UINT8_EXT; };
            return VK_INDEX_TYPE_NONE_KHR;
        };
    };

};
