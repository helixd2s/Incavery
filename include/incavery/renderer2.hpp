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
        
        vkh::uni_ptr<PipelineLayout> pipelineLayout = {};
        std::vector<vkh::uni_ptr<GraphicsPipeline>> pipelines = {};
    };


    class Renderer2 {
        protected:
        vkh::uni_ptr<vkf::Device> device = {};
        RendererInfo info = {};

        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<RendererInfo> info = RendererInfo{}) {
            this->device = device;
            this->info = info;
        };

        public:
        Renderer2(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<RendererInfo> info = RendererInfo{}) { this->constructor(device, info); };
        Renderer2() {};
    };

};
