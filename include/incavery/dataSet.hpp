#pragma once

// 
#include <glm/glm.hpp>
#include <vkf/swapchain.hpp>

// 
#include "./core.hpp"

// 
namespace icv {

    struct DataSetInfo {
        size_t count = 0ull;
        FLAGS(VkBufferUsage) usage = vkh::VkBufferUsageFlags{};
    };

    class DataSetBase: public DeviceBased
    {
        protected: 
        virtual void constructor(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<DataSetInfo> info = DataSetInfo{}) {
            this->info = info;
            this->device = device;

        };

        public:
        DataSetBase() {};
        DataSetBase(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<DataSetInfo> info = DataSetInfo{}) { this->constructor(device, info); };
    };

    template<class T = uint8_t>
    class DataSet: public DataSetBase 
    {
        protected:
        vkt::Vector<T> cpuCache = {};
        vkt::Vector<T> deviceBuffer = {};
        DataSetInfo info = {};

        // 
        virtual void constructor(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<DataSetInfo> info = DataSetInfo{}) {
            this->info = info;
            this->device = device;

            this->cpuCache = createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeof(T) * info->count, sizeof(T), VMA_MEMORY_USAGE_CPU_TO_GPU);
            this->deviceBuffer = createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|info->usage, sizeof(T) * info->count, sizeof(T), VMA_MEMORY_USAGE_GPU_ONLY);
        };
        
        public:
        DataSet() {};
        DataSet(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<DataSetInfo> info = DataSetInfo{}) { this->constructor(device, info); };

        //
        operator vkt::Vector<T>&() {
            return deviceBuffer;
        };

        //
        operator const vkt::Vector<T>&() const {
            return deviceBuffer;
        };

        //
        vkt::Vector<T>& getCpuCache() {
            return cpuCache;
        };

        //
        const vkt::Vector<T>& getCpuCache() const {
            return cpuCache;
        };

        //
        vkt::Vector<T>& getDeviceBuffer() {
            return deviceBuffer;
        };

        //
        const vkt::Vector<T>& getDeviceBuffer() const {
            return deviceBuffer;
        };

        //
        virtual void copyFromVector(const std::vector<T>& data) {
            memcpy(cpuCache.mapped(), data.data(), std::min(data.size() * sizeof(T), cpuCache.range()));
        };

        //
        virtual void cmdCopyFromCpu(VkCommandBuffer commandBuffer) {
            VkBufferCopy2KHR srcCopy = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2_KHR,
                .pNext = nullptr,
                .srcOffset = cpuCache.offset(),
                .dstOffset = deviceBuffer.offset(),
                .size = std::min(deviceBuffer.range(), deviceBuffer.range())
            };
            VkCopyBufferInfo2KHR copyInfo = {
                .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2_KHR,
                .pNext = nullptr,
                .srcBuffer = cpuCache,
                .dstBuffer = deviceBuffer,
                .regionCount = 1,
                .pRegions = &srcCopy
            };
            device->dispatch->CmdCopyBuffer2KHR(commandBuffer, &copyInfo);
        };

        //
        virtual void copyFromCpu(vkt::uni_ptr<vlf::Queue> queue) {
            queue->submitOnce([&,this](VkCommandBuffer commandBuffer){
                this->cmdCopyFromCpu(commandBuffer);
            });
        };
    };

};