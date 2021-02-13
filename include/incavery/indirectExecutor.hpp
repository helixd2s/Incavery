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

    struct IndirectExecutorInfo {
        ComputePipelineSource source = {};

        // if used for indirect draw instances level
        vkh::uni_ptr<DrawInstanceLevel> instances = {};
    };

    class IndirectExecutor {
        protected:
        vkh::uni_ptr<vkf::Device> device = {};
        vkh::uni_ptr<PipelineLayout> pipelineLayout = {};
        
        // 
        VkPipeline indirectCompute = VK_NULL_HANDLE;
        IndirectExecutorInfo info = {};

        // 
        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<IndirectExecutorInfo> info = IndirectExecutorInfo{}) {
            this->device = device;
            this->info = info;
        };

        // 
        public:
        IndirectExecutor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<IndirectExecutorInfo> info = IndirectExecutorInfo{}) { this->constructor(device, info); };
        IndirectExecutor() {};

        // 
        virtual void createPipeline() 
        {   
            if (!this->indirectCompute) {
                this->indirectCompute = vkt::createCompute(device->dispatch, info.source.path, pipelineLayout->layout, device->pipelineCache);
            };
        };

        //
        virtual void createIndirectBuffers() {
            
        };

    };
};
