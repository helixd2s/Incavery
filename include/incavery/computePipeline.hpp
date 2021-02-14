#pragma once

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"
#include "./geometryLevel.hpp"
#include "./instanceLevel.hpp"
#include "./framebuffer.hpp"

//
#include "./materialSet.hpp"
#include "./pipelineLayout.hpp"
#include "./drawInstanceLevel.hpp"

namespace icv {

    //
    struct ComputePipelineInfo {
        vkh::uni_ptr<PipelineLayout> layout = {};
        ComputePipelineSource source = {};
    };

    // 
    class ComputePipeline: public DeviceBased {
        protected: // 
        VkPipeline pipeline = VK_NULL_HANDLE;
        ComputePipelineInfo info = {};

        // 
        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<ComputePipelineInfo> info = ComputePipelineInfo{}) {
            this->device = device;
            this->info = info;
        };

        // 
        public:
        ComputePipeline(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<ComputePipelineInfo> info = ComputePipelineInfo{}) { this->constructor(device, info); };
        ComputePipeline() {};

        // 
        virtual void createPipeline() 
        {   
            if (!this->pipeline) {
                this->pipeline = vkt::createCompute(device->dispatch, info.source.path, info.layout->layout, device->pipelineCache);
            };
        };

        //
        virtual void createComputeCommand(VkCommandBuffer commandBuffer, vkh::uni_arg<glm::uvec3> workgroups = glm::uvec3(1u, 1u, 1u), vkh::uni_arg<glm::uvec4> constants = glm::uvec4(0u, 0u, 0u, 0u)) 
        {
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            // 
            if (this->pipeline) {
                device->dispatch->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                device->dispatch->CmdPushConstants(commandBuffer, info.layout->layout, pipusage, 0u, sizeof(glm::uvec4), &constants);
                device->dispatch->CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, info.layout->layout, 0u, info.layout->descriptorSets.size(), info.layout->descriptorSets.data(), 0u, nullptr);
                device->dispatch->CmdDispatch(commandBuffer, workgroups->x, workgroups->y, workgroups->z);
            } else {
                std::cerr << "Compute pipeline not initialized" << std::endl;
            };
        };

    };
};
