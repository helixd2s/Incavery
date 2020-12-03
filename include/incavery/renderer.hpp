#pragma once

// 
#include <glm/glm.hpp>
#include <vkf/swapchain.hpp>

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"
#include "./geometryLevel.hpp"
#include "./instanceLevel.hpp"

// 
namespace icv {

    struct RendererInfo
    {
        vkt::uni_ptr<GeometryRegistry> registry = {};
        vkt::uni_ptr<InstanceLevel> instanceLevel = {};
    };

    struct FramebufferInfo
    {
        std::vector<vkt::ImageRegion> images = {};
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        vkt::ImageRegion depthImage = {};
    };

    struct PipelineInfo
    {
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipeline rayTracing = VK_NULL_HANDLE;
        VkPipeline opaqueRasterization = VK_NULL_HANDLE;
        VkPipeline translucentRasterization = VK_NULL_HANDLE;
    };
    
    struct GraphicsPipelineSource 
    {
        std::string vertex = "";
        std::string geometry = "";
        std::string fragment = "";
    };

    struct ComputePipelineSource
    {
        std::string path = "";
    };

    struct PipelineCreateInfo
    {
        ComputePipelineSource rayTracing = { "./shaders/rayTracing.comp.spv" };
        GraphicsPipelineSource opaqueRasterization = {
            "./shaders/opaque.vert.spv",
            "./shaders/opaque.geom.spv",
            "./shaders/opaque.frag.spv"
        };
        GraphicsPipelineSource translucentRasterization = {
            "./shaders/translucent.vert.spv",
            "./shaders/translucent.geom.spv",
            "./shaders/translucent.frag.spv"
        };
    };

    class Renderer: public DeviceBased
    {
        protected: 
        RendererInfo info = {};
        FramebufferInfo framebuffer = {};
        PipelineInfo pipeline = {};

        // 
        VkDescriptorSetLayout framebufferLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout geometryRegistryLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout instanceLevelLayout = VK_NULL_HANDLE;
        
        // 
        VkRenderPass renderPass = VK_NULL_HANDLE;

        // 
        


        constexpr uint32_t FBO_COUNT = 4u;

        virtual void constructor(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<RendererInfo> info = RendererInfo{}) 
        {
            this->info = info;
            this->device = device;

        };

        public:
        Renderer() {};
        Renderer(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<RendererInfo> info = RendererInfo{}) { this->constructor(device, info); };

        //
        virtual VkDescriptorSetLayout& getInstanceLevelLayout() 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            if (!instanceLevelLayout) 
            {   // create descriptor set layout
                vkh::VsDescriptorSetLayoutCreateInfoHelper descriptorSetLayoutHelper(vkh::VkDescriptorSetLayoutCreateInfo{});
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 0u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1, .eVariableDescriptorCount = 1 });
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 1u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                    .descriptorCount = 1u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1, .eVariableDescriptorCount = 1 });
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 2u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 256u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1, .eVariableDescriptorCount = 1 });
                vkh::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &instanceLevelLayout));
            };

            return instanceLevelLayout;
        };

        //
        virtual VkDescriptorSetLayout& getGeometryRegistryLayout() 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            if (!geometryRegistryLayout) 
            {   // create descriptor set layout
                vkh::VsDescriptorSetLayoutCreateInfoHelper descriptorSetLayoutHelper(vkh::VkDescriptorSetLayoutCreateInfo{});
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 0u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 256u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1, .eVariableDescriptorCount = 1 });
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 1u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1, .eVariableDescriptorCount = 1 });
                vkh::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &geometryRegistryLayout));
            };

            return geometryRegistryLayout;
        };

        //
        virtual VkDescriptorSetLayout& getFramebufferLayout() 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            if (!framebufferLayout) 
            {   // create descriptor set layout
                vkh::VsDescriptorSetLayoutCreateInfoHelper descriptorSetLayoutHelper(vkh::VkDescriptorSetLayoutCreateInfo{});
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 0u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = FBO_COUNT,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1, .eVariableDescriptorCount = 1 });
                vkh::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &framebufferLayout));
            };

            return framebufferLayout;
        };


        //
        virtual const VkDescriptorSetLayout& getInstanceLevelLayout() const 
        {   // 
            return instanceLevelLayout;
        };

        //
        virtual const VkDescriptorSetLayout& getGeometryRegistryLayout() const 
        {   // 
            return geometryRegistryLayout;
        };

        //
        virtual const VkDescriptorSetLayout& getFramebufferLayout() const 
        {   // 
            return framebufferLayout;
        };


        //
        virtual VkPipelineLayout& createPipelineLayout(std::vector<VkDescriptorSetLayout> layouts = {}) 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            // 
            layouts.insert(layouts.begin(), this->getFramebufferLayout());
            layouts.insert(layouts.begin(), this->getGeometryRegistryLayout());
            layouts.insert(layouts.begin(), this->getInstanceLevelLayout());

            // 
            std::vector<vkh::VkPushConstantRange> rtRanges = { vkh::VkPushConstantRange{.stageFlags = pipusage, .offset = 0u, .size = 16u } };
            vkh::handleVk(device->dispatch->CreatePipelineLayout(vkh::VkPipelineLayoutCreateInfo{  }.setSetLayouts(layouts).setPushConstantRanges(rtRanges), nullptr, &pipeline.layout));
        };

        // 
        virtual VkRenderPass& createRenderPass() 
        {   // 
            auto render_pass_helper = vkh::VsRenderPassCreateInfoHelper();

            for (uint32_t i=0;i<FBO_COUNT;i++) 
            {
                render_pass_helper.addColorAttachment(vkh::VkAttachmentDescription
                {
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .finalLayout = VK_IMAGE_LAYOUT_GENERAL
                });
            };

            render_pass_helper.setDepthStencilAttachment(vkh::VkAttachmentDescription
            {
                .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            });

            auto dp0 = vkh::VkSubpassDependency
            {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0u,
            };

            auto dp1 = vkh::VkSubpassDependency
            {
                .srcSubpass = 0u,
                .dstSubpass = VK_SUBPASS_EXTERNAL,
            };

            {   // 
                auto srcStageMask = vkh::VkPipelineStageFlags{ .eColorAttachmentOutput = 1, .eTransfer = 1, .eBottomOfPipe = 1, };   ASSIGN(dp0, srcStageMask);
                auto dstStageMask = vkh::VkPipelineStageFlags{ .eColorAttachmentOutput = 1, };                                       ASSIGN(dp0, dstStageMask);
                auto srcAccessMask = vkh::VkAccessFlags{ .eColorAttachmentWrite = 1 };                                               ASSIGN(dp0, srcAccessMask);
                auto dstAccessMask = vkh::VkAccessFlags{ .eColorAttachmentRead = 1, .eColorAttachmentWrite = 1 };                    ASSIGN(dp0, dstAccessMask);
                auto dependencyFlags = vkh::VkDependencyFlags{ .eByRegion = 1 };                                                     ASSIGN(dp0, dependencyFlags);
            };

            {   // 
                auto srcStageMask = vkh::VkPipelineStageFlags{ .eColorAttachmentOutput = 1 };                                         ASSIGN(dp1, srcStageMask);
                auto dstStageMask = vkh::VkPipelineStageFlags{ .eTopOfPipe = 1, .eColorAttachmentOutput = 1, .eTransfer = 1 };        ASSIGN(dp1, dstStageMask);
                auto srcAccessMask = vkh::VkAccessFlags{ .eColorAttachmentRead = 1, .eColorAttachmentWrite = 1 };                     ASSIGN(dp1, srcAccessMask);
                auto dstAccessMask = vkh::VkAccessFlags{ .eColorAttachmentRead = 1, .eColorAttachmentWrite = 1 };                     ASSIGN(dp1, dstAccessMask);
                auto dependencyFlags = vkh::VkDependencyFlags{ .eByRegion = 1 };                                                      ASSIGN(dp1, dependencyFlags);
            };

            // 
            render_pass_helper.addSubpassDependency(dp0);
            render_pass_helper.addSubpassDependency(dp1);

            // 
            vkh::handleVk(device->dispatch->CreateRenderPass(render_pass_helper, nullptr, &renderPass));
            return renderPass;
        };

        //
        virtual VkRenderPass& getRenderPass() 
        {   // 
            if (!renderPass) { this->createRenderPass(); };
            return renderPass;
        };

        //
        virtual VkPipelineLayout& getPipelineLayout() 
        {   // 
            if (!pipelineLayout) { this->createPipelineLayout(); };
            return pipelineLayout;
        };

        // 
        virtual FramebufferInfo& createFramebuffer(vkh::VkExtent3D size = {})
        {   //
            std::vector<VkImageView> views = {};

            for (uint32_t i=0;i<FBO_COUNT;i++) 
            {
                framebuffer.images.push_back(createImage2D(VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, size, false));
                views.push_back(framebuffer.images.back());
            };

            {
                framebuffer.depthImage = createImage2D(VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, size, true);
                views.push_back(framebuffer.depthImage);
            };

            vkh::handleVk(device->dispatch->CreateFramebuffer(vkh::VkFramebufferCreateInfo{ .flags = {}, .renderPass = renderpass, .attachmentCount = uint32_t(views.size()), .pAttachments = views.data(), .width = size.x, .height = size.y, .layers = 1u }, nullptr, &framebuffer.framebuffer));

            return framebuffer;
        };

        // 
        virtual PipelineInfo& createPipeline(vkt::uni_arg<PipelineCreateInfo> info = PipelineCreateInfo{}) 
        {   // 
            
        };

    };

};