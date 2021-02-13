#pragma once

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"
#include "./geometryLevel.hpp"
#include "./instanceLevel.hpp"
#include "./framebuffer.hpp"

//
#include "./materialSet.hpp"

// 
namespace icv {

    // 
    struct DefinedDescriptorSetLayout {
        VkDescriptorSetLayout framebuffer = VK_NULL_HANDLE;
        VkDescriptorSetLayout geometryRegistry = VK_NULL_HANDLE;
        VkDescriptorSetLayout instanceLevel = VK_NULL_HANDLE;
        VkDescriptorSetLayout materialSet = VK_NULL_HANDLE;
    };

    class PipelineLayout {
        public:
        vkh::uni_ptr<vkf::Device> device = {};
        DefinedDescriptorSetLayout definedLayouts = {};
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;

        public: 

        // 
        virtual VkRenderPass& createRenderPass() 
        {   // 
            return Framebuffer::createRenderPass(device, renderPass);
        };

        //
        template<class M = MaterialSource>
        VkDescriptorSetLayout& getMaterialSetLayout() 
        {   //
            return MaterialSet<M>::createDescriptorSetLayout(device, definedLayouts.materialSet);
        };

        //
        VkDescriptorSetLayout& getInstanceLevelLayout() 
        {   //
            return InstanceLevel::createDescriptorSetLayout(device, definedLayouts.instanceLevel);
        };

        //
        VkDescriptorSetLayout& getGeometryRegistryLayout() 
        {   //
            return GeometryRegistry::createDescriptorSetLayout(device, definedLayouts.geometryRegistry);
        };

        //
        VkDescriptorSetLayout& getFramebufferLayout() 
        {   //
            return Framebuffer::createDescriptorSetLayout(device, definedLayouts.framebuffer);
        };

        //
        template<class M = MaterialSource>
        VkPipelineLayout& createPipelineLayout(std::vector<VkDescriptorSetLayout> layouts = {}) 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            // 
            layouts.insert(layouts.begin(), this->getMaterialSetLayout<M>());
            layouts.insert(layouts.begin(), this->getInstanceLevelLayout()); // 3th
            layouts.insert(layouts.begin(), this->getGeometryRegistryLayout()); // secondly
            layouts.insert(layouts.begin(), this->getFramebufferLayout()); // firstly

            // 
            std::vector<vkh::VkPushConstantRange> rtRanges = { vkh::VkPushConstantRange{.stageFlags = pipusage, .offset = 0u, .size = 16u } };
            vkt::handleVk(device->dispatch->CreatePipelineLayout(vkh::VkPipelineLayoutCreateInfo{  }.setSetLayouts(layouts).setPushConstantRanges(rtRanges), nullptr, &layout));
            return layout;
        };
    };

};
