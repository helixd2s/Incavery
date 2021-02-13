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
        vkh::uni_ptr<PipelineLayout> layout = {};
        GraphicsPipelineSource source = {};
        
        vkh::VkViewport viewport = {};
        vkh::VkRect2D scissor = {};
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
                pipelineInfo.graphicsPipelineCreateInfo.renderPass = info.layout->renderPass;
                pipelineInfo.viewportState.pViewports = &reinterpret_cast<::VkViewport&>(info.viewport);
                pipelineInfo.viewportState.pScissors = &reinterpret_cast<::VkRect2D&>(info.scissor);
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

    };

};
