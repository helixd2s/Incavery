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

// 
namespace icv {


    struct RendererInfo
    {
        vkh::uni_ptr<Framebuffer> framebuffer = {};
        vkh::uni_ptr<GeometryRegistry> geometryRegistry = {};
        vkh::uni_ptr<InstanceLevel> instanceLevel = {};
        vkh::uni_ptr<MaterialSetBase> materialSet = {};
        vkh::uni_ptr<DrawInstanceLevel> drawInstanceLevel = {};
        
        std::vector<vkh::uni_ptr<GraphicsPipeline>> pipelines = {};
        std::vector<vkh::uni_ptr<GeometryLevel>> geometries = {};
    };


    class Renderer2 {
        protected:
        vkh::uni_ptr<vkf::Device> device = {};
        RendererInfo info = {};

        // 
        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<RendererInfo> info = RendererInfo{}) {
            this->device = device;
            this->info = info;
        };

        // 
        public:
        Renderer2(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<RendererInfo> info = RendererInfo{}) { this->constructor(device, info); };
        Renderer2() {};


        //
        virtual uintptr_t changeGeometry(uintptr_t geometryId, vkh::uni_ptr<GeometryLevel> geometryLevel = {})
        {   // add instance into registry
            if (this->info.geometries.size() <= geometryId) { this->info.geometries.resize(geometryId + 1u); };
            this->info.geometries[geometryId] = geometryLevel;
            return geometryId;
        };

        //
        virtual uintptr_t pushGeometry(vkh::uni_ptr<GeometryLevel> info = {})
        {   // add instance into registry
            uintptr_t geometryId = this->info.geometries.size();
            this->info.geometries.push_back(info);
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
            uintptr_t pipelineId = this->info.geometries.size();
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
            this->info.drawInstanceLevel->setGeometryReferences(this->info.geometries);
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
            auto instanceLevelInfo = info.drawInstanceLevel->getInfo();
            for (uint32_t I=0;I<instanceLevelInfo.instances.size();I++) {
                auto& instanceInfo = instanceLevelInfo.instances[I];

                // getting hosted data (backward compatibility)
                auto& geometryLevelInfo = info.geometries[instanceInfo.geometryLevelId]->getInfo();

                // 
                for (uint32_t G=0;G<geometryLevelInfo.geometries.size();G++) {
                    info.pipelines[instanceInfo.programId]->createRenderingCommand(commandBuffer, info.framebuffer, glm::uvec4{I, G, 0u, 0u}, geometryLevelInfo.geometries[G].primitive);
                };

                // TODO: instances support (needs pre-compute shader)
                //info.pipelines[instanceInfo.programId]->createRenderingCommand(commandBuffer, info.framebuffer, glm::uvec4{I, 0u, 0u, 0u}, info.drawInstanceLevel);

            };
            vkt::commandBarrier(device->dispatch, commandBuffer);
        };


    };

};
