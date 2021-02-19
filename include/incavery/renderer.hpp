#pragma once

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"
#include "./geometryLevel.hpp"
#include "./instanceLevel.hpp"
#include "./framebuffer.hpp"

//
#include "./drawInstanceLevel.hpp"
#include "./materialSet.hpp"
#include "./pipelineLayout.hpp"
#include "./graphicsPipeline.hpp"
#include "./computePipeline.hpp"

// 
namespace icv {

    // 
    struct RendererInfo
    {   // 
        vkh::uni_ptr<Framebuffer> framebuffer = {};
        vkh::uni_ptr<InstanceLevel> instanceLevel = {};
        vkh::uni_ptr<DrawInstanceLevel> drawInstanceLevel = {};

        // reserved for future usage (currently are descriptor set sources)
        vkh::uni_ptr<GeometryRegistry> geometryRegistry = {};
        vkh::uni_ptr<MaterialSetBase> materialSet = {};

        // needs for some related operations
        std::vector<vkh::uni_ptr<GeometryLevel>> geometryLevels = {};

        // pipelines
        vkh::uni_ptr<ComputePipeline> indirectCompute = {};
        std::vector<vkh::uni_ptr<GraphicsPipeline>> pipelines = {};
        vkh::uni_ptr<ComputePipeline> rayTraceCompute = {};
    };

    // 
    class Renderer: public DeviceBased {
        protected:
        RendererInfo info = {};

        // 
        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<RendererInfo> info = RendererInfo{}) {
            this->device = device;
            this->info = info;
        };

        // 
        public:
        Renderer(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<RendererInfo> info = RendererInfo{}) { this->constructor(device, info); };
        Renderer() {};


        //
        virtual void changeRayTracingPipeline(vkh::uni_ptr<ComputePipeline> computePipeline = {}) {
            this->info.rayTraceCompute = computePipeline;
        };


        //
        virtual uintptr_t changeGeometryLevel(uintptr_t geometryId, vkh::uni_ptr<GeometryLevel> geometryLevel = {})
        {   // add instance into registry
            if (this->info.geometryLevels.size() <= geometryId) { this->info.geometryLevels.resize(geometryId + 1u); };
            this->info.geometryLevels[geometryId] = geometryLevel;
            return geometryId;
        };

        //
        virtual uintptr_t pushGeometryLevel(vkh::uni_ptr<GeometryLevel> info = {})
        {   // add instance into registry
            uintptr_t geometryId = this->info.geometryLevels.size();
            this->info.geometryLevels.push_back(info);
            return geometryId;
        };



        //
        virtual uintptr_t changeGraphicsPipeline(uintptr_t pipelineId, vkh::uni_ptr<GraphicsPipeline> graphicsPipeline = {})
        {   // add instance into registry
            if (this->info.pipelines.size() <= pipelineId) { this->info.pipelines.resize(pipelineId + 1u); };
            this->info.pipelines[pipelineId] = graphicsPipeline;
            return pipelineId;
        };

        //
        virtual uintptr_t pushGraphicsPipeline(vkh::uni_ptr<GraphicsPipeline> graphicsPipeline = {})
        {   // add instance into registry
            uintptr_t pipelineId = this->info.pipelines.size();
            this->info.pipelines.push_back(graphicsPipeline);
            return pipelineId;
        };


        //
        virtual void setFramebuffer(vkh::uni_ptr<Framebuffer> framebuffer) 
        {
            this->info.framebuffer = framebuffer;
        };

        //
        virtual void setMaterialSet(vkh::uni_ptr<MaterialSetBase> materialSet) 
        {
            this->info.materialSet = materialSet;
        };

        // 
        virtual void setGeometryReferences() 
        {
            this->info.drawInstanceLevel->setGeometryReferences(this->info.geometryLevels);
            this->info.instanceLevel->setGeometryReferences(this->info.geometryLevels);
        };


        //
        virtual void createRenderingCommand(VkCommandBuffer commandBuffer) 
        {
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            // clear framebuffers
            auto& framebuffer = info.framebuffer->getState();
            {
                for (auto& image : framebuffer.images) {
                    image.transfer(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                    device->dispatch->CmdClearColorImage(commandBuffer, image, image.getImageLayout(), vkh::VkClearColorValue{ .float32 = { 0.f,0.f,0.f,0.f } }, 1u, image.getImageSubresourceRange());
                    image.transfer(commandBuffer, VK_IMAGE_LAYOUT_GENERAL);
                };
                {
                    framebuffer.depthImage.transfer(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                    device->dispatch->CmdClearDepthStencilImage(commandBuffer, framebuffer.depthImage, framebuffer.depthImage.getImageLayout(), vkh::VkClearDepthStencilValue{ .depth = 1.0f, .stencil = 0 }, 1u, framebuffer.depthImage.getImageSubresourceRange());
                    framebuffer.depthImage.transfer(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
                };
                vkt::commandBarrier(device->dispatch, commandBuffer);
            };

            // select opaque and translucent for draw
            if (info.drawInstanceLevel.has()) {
                auto instanceLevelInfo = info.drawInstanceLevel->getInfo();

                // compute indirect operations
                if (info.indirectCompute.has()) {
                    info.indirectCompute->createComputeCommand(commandBuffer, glm::uvec3(instanceLevelInfo.instances.size(), 1u, 1u), glm::uvec4(0u));
                    vkt::commandBarrier(device->dispatch, commandBuffer);
                } else {
                    std::cerr << "Indirect compute not defined" << std::endl;
                };

                // 
                for (uint32_t I=0;I<instanceLevelInfo.instances.size();I++) {
                    auto& instanceInfo = instanceLevelInfo.instances[I];

                    // using geometry levels descriptions for draw
                    auto& geometryLevelInfo = info.geometryLevels[instanceInfo.geometryLevelId]->getInfo();
                    for (uint32_t G=0;G<geometryLevelInfo.geometries.size();G++) {
                        DrawInfo drawInfo = DrawInfo{0u, 0u, PushConstantInfo{I, G, 0u, 0u}, geometryLevelInfo.geometries[G].primitive};
                        info.pipelines[instanceInfo.programId]->createRenderingCommand(commandBuffer, info.framebuffer, drawInfo);
                    };

                    // TODO: instances support (needs pre-compute shader)
                    //info.pipelines[instanceInfo.programId]->createRenderingCommand(commandBuffer, info.framebuffer, info.drawInstanceLevel, I);
                };

                //
                if (instanceLevelInfo.instances.size() > 0u) {
                    vkt::commandBarrier(device->dispatch, commandBuffer);
                };
            } else {
                std::cerr << "Draw instances not defined" << std::endl;
            };

            // compute ray tracing
            if (info.rayTraceCompute.has()) {
                const uint32_t LOCAL_GROUP_X = 32u, LOCAL_GROUP_Y = 24u;
                info.rayTraceCompute->createComputeCommand(commandBuffer, glm::uvec3(framebuffer.scissor.extent.width/LOCAL_GROUP_X, framebuffer.scissor.extent.height/LOCAL_GROUP_Y, 1u), glm::uvec4(0u));
                vkt::commandBarrier(device->dispatch, commandBuffer);
            } else {
                std::cerr << "Ray tracing compute not defined" << std::endl;
            };
        };


    };

};
