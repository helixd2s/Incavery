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
        DataSetInfo info = {};

        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<DataSetInfo> info = DataSetInfo{}) {
            this->info = info;
            this->device = device;

        };

        public:
        DataSetBase() {};
        DataSetBase(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<DataSetInfo> info = DataSetInfo{}) { this->constructor(device, info); };
    };

    template<class T = uint8_t>
    class DataSet: public DataSetBase 
    {
        protected:
        vkf::Vector<T> cpuCache = {};
        vkf::Vector<T> deviceBuffer = {};
        

        // 
        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<DataSetInfo> info = DataSetInfo{}) {
            this->info = info;
            this->device = device;

            // 
            this->cpuCache = vkf::Vector<T>(createBuffer(BufferCreateInfo{ .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, .size = sizeof(T) * info->count, .stride = sizeof(T), .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU }));
            this->deviceBuffer = vkf::Vector<T>(createBuffer(BufferCreateInfo{ .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VkBufferUsageFlags(info->usage), .size = sizeof(T) * info->count, .stride = sizeof(T), .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY }));
        };
        
        public:
        DataSet() {};
        DataSet(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<DataSetInfo> info = DataSetInfo{}) { this->constructor(device, info); };

        //
        operator vkf::Vector<T>&() {
            return deviceBuffer;
        };

        //
        operator const vkf::Vector<T>&() const {
            return deviceBuffer;
        };

        //
        vkf::Vector<T>& getCpuCache() {
            return cpuCache;
        };

        //
        const vkf::Vector<T>& getCpuCache() const {
            return cpuCache;
        };

        //
        vkf::Vector<T>& getDeviceBuffer() {
            return deviceBuffer;
        };

        //
        const vkf::Vector<T>& getDeviceBuffer() const {
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
        virtual void copyFromCpu(vkh::uni_ptr<vkf::Queue> queue) {
            queue->submitOnce([&,this](VkCommandBuffer commandBuffer){
                this->cmdCopyFromCpu(commandBuffer);
            });
        };
    };

};
