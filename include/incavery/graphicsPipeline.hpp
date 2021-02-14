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

// 
namespace icv {

    // 
    struct GraphicsPipelineInfo {
        vkh::uni_ptr<Framebuffer> framebuffer = {};
        vkh::uni_ptr<PipelineLayout> layout = {};
        GraphicsPipelineSource source = {};
    };

    // 
    struct PushConstantInfo 
    {
        uint32_t instanceId = 0u;
        uint32_t geometryId = 0u;
        uint32_t reserved0 = 0u;
        uint32_t reserved1 = 0u;
    };

    // 
    struct DrawInfo
    {
        uint32_t type = 0u;
        uint32_t reserved0 = 0u;
        PushConstantInfo constants = {};
        PrimitiveInfo primitive = {};
    };

    // 
    class GraphicsPipeline {
        protected:
        vkh::uni_ptr<vkf::Device> device = {};
        
        VkPipeline pipeline = VK_NULL_HANDLE;
        GraphicsPipelineInfo info = {};

        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<GraphicsPipelineInfo> info = GraphicsPipelineInfo{}) {
            this->device = device;
            this->info = info;
        };

        public:
        GraphicsPipeline(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<GraphicsPipelineInfo> info = GraphicsPipelineInfo{}) { this->constructor(device, info); };
        GraphicsPipeline() {};

        // 
        virtual void createPipeline() 
        {   
            // 
            vkh::VsGraphicsPipelineCreateInfoConstruction pipelineInfo = {};
            {   // initial state
                pipelineInfo.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                pipelineInfo.graphicsPipelineCreateInfo.layout = info.layout->layout;
                pipelineInfo.graphicsPipelineCreateInfo.renderPass = info.framebuffer->getInfo().renderPass;
                pipelineInfo.viewportState.pViewports = &reinterpret_cast<::VkViewport&>(info.framebuffer->getState().viewport);
                pipelineInfo.viewportState.pScissors = &reinterpret_cast<::VkRect2D&>(info.framebuffer->getState().scissor);
                pipelineInfo.colorBlendAttachmentStates = {};
                pipelineInfo.dynamicStates = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT };
                pipelineInfo.depthStencilState = vkh::VkPipelineDepthStencilStateCreateInfo{ .depthTestEnable = true, .depthWriteEnable = true };
            };

            // blend states
            for (uint32_t i=0;i<Framebuffer::FBO_COUNT;i++) 
            {   // TODO: full blending support
                pipelineInfo.colorBlendAttachmentStates.push_back(vkh::VkPipelineColorBlendAttachmentState{
                    .blendEnable = false
                });
            };

            // opaque rasterization
            pipelineInfo.stages = {
                vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info.source.vertex), VK_SHADER_STAGE_VERTEX_BIT),
                vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info.source.geometry), VK_SHADER_STAGE_GEOMETRY_BIT),
                vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info.source.fragment), VK_SHADER_STAGE_FRAGMENT_BIT)
            };
            vkt::handleVk(device->dispatch->CreateGraphicsPipelines(device->pipelineCache, 1u, pipelineInfo, nullptr, &pipeline));
        };

        // indirect draw support
        // required compute shader for pre-compute indirect draw
        virtual void createRenderingCommand(VkCommandBuffer commandBuffer, vkh::uni_ptr<Framebuffer> framebuffer = {}, vkh::uni_ptr<DrawInstanceLevel> drawInstanceLevel = {}, const uint32_t instanceId = 0u) 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            // TODO: use with framebuffer as native
            std::vector<vkh::VkClearValue> clearValues = {};
            for (uint32_t i=0;i<Framebuffer::FBO_COUNT;i++) {
                clearValues.push_back(vkh::VkClearColorValue{});
                clearValues.back().color.float32 = glm::vec4(0.f, 0.f, 0.f, 0.f);
            };
            clearValues.push_back(vkh::VkClearDepthStencilValue{ 1.0f, 0 });

            // 
            if (this->pipeline) {
                PushConstantInfo constants = PushConstantInfo{instanceId, 0u, 0u, 0u};
                auto& instanceInfo = drawInstanceLevel->getInfo().instances[instanceId];
                device->dispatch->CmdBeginRenderPass(commandBuffer, vkh::VkRenderPassBeginInfo{ .renderPass = info.framebuffer->getInfo().renderPass, .framebuffer = framebuffer->getState().framebuffer, .renderArea = framebuffer->getState().scissor, .clearValueCount = uint32_t(clearValues.size()), .pClearValues = reinterpret_cast<vkh::VkClearValue*>(clearValues.data()) }, VK_SUBPASS_CONTENTS_INLINE);
                device->dispatch->CmdSetViewport(commandBuffer, 0u, 1u, framebuffer->getState().viewport);
                device->dispatch->CmdSetScissor(commandBuffer, 0u, 1u, framebuffer->getState().scissor);
                device->dispatch->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                device->dispatch->CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.layout->layout, 0u, info.layout->descriptorSets.size(), info.layout->descriptorSets.data(), 0u, nullptr);
                device->dispatch->CmdPushConstants(commandBuffer, info.layout->layout, pipusage, 0u, sizeof(PushConstantInfo), &constants);
                device->dispatch->CmdDrawIndirect(commandBuffer, drawInstanceLevel->getIndirectDrawBuffer(instanceId), 0ull, instanceInfo.geometryLevelCount, sizeof(VkDrawIndirectCommand));
                device->dispatch->CmdEndRenderPass(commandBuffer);
            } else {
                std::cerr << "Graphics pipeline not initialized" << std::endl;
            };
        };

        // 
        virtual void createRenderingCommand(VkCommandBuffer commandBuffer, vkh::uni_ptr<Framebuffer> framebuffer = {}, vkh::uni_arg<DrawInfo> drawInfo = DrawInfo{}) 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            // TODO: use with framebuffer as native
            std::vector<vkh::VkClearValue> clearValues = {};
            for (uint32_t i=0;i<Framebuffer::FBO_COUNT;i++) {
                clearValues.push_back(vkh::VkClearColorValue{});
                clearValues.back().color.float32 = glm::vec4(0.f, 0.f, 0.f, 0.f);
            };
            clearValues.push_back(vkh::VkClearDepthStencilValue{ 1.0f, 0 });

            // 
            if (this->pipeline) {
                device->dispatch->CmdBeginRenderPass(commandBuffer, vkh::VkRenderPassBeginInfo{ .renderPass = info.framebuffer->getInfo().renderPass, .framebuffer = framebuffer->getState().framebuffer, .renderArea = framebuffer->getState().scissor, .clearValueCount = uint32_t(clearValues.size()), .pClearValues = reinterpret_cast<vkh::VkClearValue*>(clearValues.data()) }, VK_SUBPASS_CONTENTS_INLINE);
                device->dispatch->CmdSetViewport(commandBuffer, 0u, 1u, framebuffer->getState().viewport);
                device->dispatch->CmdSetScissor(commandBuffer, 0u, 1u, framebuffer->getState().scissor);
                device->dispatch->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                device->dispatch->CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.layout->layout, 0u, info.layout->descriptorSets.size(), info.layout->descriptorSets.data(), 0u, nullptr);
                device->dispatch->CmdPushConstants(commandBuffer, info.layout->layout, pipusage, 0u, sizeof(PushConstantInfo), &drawInfo->constants);
                device->dispatch->CmdDraw(commandBuffer, drawInfo->primitive.count * 3u, 1u, 0u, 0u);
                device->dispatch->CmdEndRenderPass(commandBuffer);
            } else {
                std::cerr << "Graphics pipeline not initialized" << std::endl;
            };
        };

    };

};
