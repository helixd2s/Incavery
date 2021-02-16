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

// 
namespace icv {

    // 
    struct DefinedDescriptorSetLayout {
        VkDescriptorSetLayout framebuffer = VK_NULL_HANDLE;
        VkDescriptorSetLayout geometryRegistry = VK_NULL_HANDLE;
        VkDescriptorSetLayout instanceLevel = VK_NULL_HANDLE;
        VkDescriptorSetLayout drawInstanceLevel = VK_NULL_HANDLE;
        VkDescriptorSetLayout materialSet = VK_NULL_HANDLE;
    };

    // 
    struct DescriptorSetSources {
        vkh::uni_ptr<Framebuffer> framebuffer = {};
        vkh::uni_ptr<GeometryRegistry> geometryRegistry = {};
        vkh::uni_ptr<InstanceLevel> instanceLevel = {};
        vkh::uni_ptr<MaterialSetBase> materialSet = {};
        vkh::uni_ptr<DrawInstanceLevel> drawInstanceLevel = {};
    };

    //
    struct PipelineLayoutInfo {

    };

    // 
    class PipelineLayout: public DeviceBased {
        public:
        PipelineLayoutInfo info = {};
        DefinedDescriptorSetLayout definedLayouts = {};
        VkPipelineLayout layout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets = {};


        // 
        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<PipelineLayoutInfo> info = PipelineLayoutInfo{}) {
            this->device = device;
            this->info = info;
        };

        // 
    public:
        PipelineLayout(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<PipelineLayoutInfo> info = PipelineLayoutInfo{}) { this->constructor(device, info); };
        PipelineLayout() {};

        //
        virtual std::vector<VkDescriptorSet>& makeDescriptorSets(vkh::uni_arg<DescriptorSetSources> info = DescriptorSetSources{})
        {   //
            if (this->descriptorSets.size() < 5u) { this->descriptorSets.resize(4u); };
            this->descriptorSets[0u] = info->framebuffer->makeDescriptorSet( DescriptorInfo{ .layout = definedLayouts.framebuffer } );
            this->descriptorSets[1u] = info->geometryRegistry->makeDescriptorSet( DescriptorInfo{ .layout = definedLayouts.geometryRegistry } );
            this->descriptorSets[2u] = info->instanceLevel->makeDescriptorSet( DescriptorInfo{ .layout = definedLayouts.instanceLevel } );
            this->descriptorSets[3u] = info->drawInstanceLevel->makeDescriptorSet( DescriptorInfo{ .layout = definedLayouts.drawInstanceLevel } );
            this->descriptorSets[4u] = info->materialSet->makeDescriptorSet( DescriptorInfo{ .layout = definedLayouts.materialSet } );
            return this->descriptorSets;
        };

        //
        VkDescriptorSetLayout& getMaterialSetLayout() 
        {   //
            return MaterialSetBase::createDescriptorSetLayout(device, definedLayouts.materialSet);
        };

        //
        VkDescriptorSetLayout& getDrawInstanceLevelLayout() 
        {   //
            return DrawInstanceLevel::createDescriptorSetLayout(device, definedLayouts.drawInstanceLevel);
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
        VkPipelineLayout& createPipelineLayout(std::vector<VkDescriptorSetLayout> layouts = {}) 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            // 
            layouts.insert(layouts.begin(), this->getMaterialSetLayout());
            layouts.insert(layouts.begin(), this->getDrawInstanceLevelLayout()); // 3th
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
