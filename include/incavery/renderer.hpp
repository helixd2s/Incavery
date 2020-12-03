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

        // planned multiple viewports
        vkh::VkViewport viewport = {};
        vkh::VkRect2D scissor = {};
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

        // 
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
            auto renderPassHelper = vkh::VsRenderPassCreateInfoHelper();

            for (uint32_t i=0;i<FBO_COUNT;i++) 
            {
                renderPassHelper.addColorAttachment(vkh::VkAttachmentDescription
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

            renderPassHelper.setDepthStencilAttachment(vkh::VkAttachmentDescription
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
            renderPassHelper.addSubpassDependency(dp0);
            renderPassHelper.addSubpassDependency(dp1);

            // 
            vkh::handleVk(device->dispatch->CreateRenderPass(renderPassHelper, nullptr, &renderPass));
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
            {   // 
                framebuffer.images.push_back(createImage2D(VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, size, false));
                views.push_back(framebuffer.images.back());
            };

            {   // 
                framebuffer.depthImage = createImage2D(VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, size, true);
                views.push_back(framebuffer.depthImage);
            };

            vkh::handleVk(device->dispatch->CreateFramebuffer(vkh::VkFramebufferCreateInfo{ .flags = {}, .renderPass = renderpass, .attachmentCount = uint32_t(views.size()), .pAttachments = views.data(), .width = size.x, .height = size.y, .layers = 1u }, nullptr, &framebuffer.framebuffer));

            {   // 
                framebuffer.scissor = vkh::VkRect2D{ vkh::VkOffset2D{0, 0}, vkh::VkExtent2D{ size.x, size.y } };
                framebuffer.viewport = vkh::VkViewport{ 0.0f, 0.0f, static_cast<float>(size.x), static_cast<float>(size.y), 0.f, 1.f };
            };

            return framebuffer;
        };

        // 
        virtual PipelineInfo& createPipeline(vkt::uni_arg<PipelineCreateInfo> info = PipelineCreateInfo{}) 
        {   // 
            pipeline.rayTracing = vkt::createCompute(device->dispatch, info->rayTracing.path, this->getPipelineLayout(), device->pipelineCache, 32u);

            // 
            vkh::VsGraphicsPipelineCreateInfoConstruction pipelineInfo = {};
            {   // initial state
                pipelineInfo.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                pipelineInfo.graphicsPipelineCreateInfo.layout = this->getPipelineLayout();
                pipelineInfo.graphicsPipelineCreateInfo.renderPass = this->getRenderPass();
                pipelineInfo.viewportState.pViewports = &reinterpret_cast<::VkViewport&>(framebuffer.viewport);
                pipelineInfo.viewportState.pScissors = &reinterpret_cast<::VkRect2D&>(framebuffer.scissor);
                pipelineInfo.colorBlendAttachmentStates = {};
                pipelineInfo.dynamicStates = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT };
            };

            //
            {   // vertex input for dynamic bindings
                pipelineInfo.vertexInputAttributeDescriptions.push_back(vkh::VkVertexInputAttributeDescription
                {
                    .location = 0u, 
                    .binding = 0u,
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = 0u // infeasible with dynamic state bindings
                });
                pipelineInfo.vertexInputBindingDescriptions.push_back(vkh::VkVertexInputBindingDescription
                {
                    .binding = 0u,
                    .stride = 16u, // can be changed
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX 
                });
            }

            // blend states
            for (uint32_t i=0;i<FBO_COUNT;i++) 
            {   // TODO: full blending support
                pipelineInfo.colorBlendAttachmentStates.push_back(vkh::VkPipelineColorBlendAttachmentState{
                    .blendEnable = false
                });
            };

            {   // opaque rasterization
                pipelineInfo.stages = {
                    vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info->opaque.vertex), VK_SHADER_STAGE_VERTEX_BIT),
                    vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info->opaque.geometry), VK_SHADER_STAGE_GEOMETRY_BIT),
                    vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info->opaque.fragment), VK_SHADER_STAGE_FRAGMENT_BIT)
                };
                vkh::handleVk(device->dispatch->CreateGraphicsPipelines(device->pipelineCache, 1u, pipelineInfo, nullptr, &opaqueRasterization));
            };

            {   // translucent rasterization
                pipelineInfo.stages = {
                    vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info->translucent.vertex), VK_SHADER_STAGE_VERTEX_BIT),
                    vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info->translucent.geometry), VK_SHADER_STAGE_GEOMETRY_BIT),
                    vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info->translucent.fragment), VK_SHADER_STAGE_FRAGMENT_BIT)
                };
                vkh::handleVk(device->dispatch->CreateGraphicsPipelines(device->pipelineCache, 1u, pipelineInfo, nullptr, &translucentRasterization);
            };

            return pipeline;
        };

        // 
        virtual void createRenderingCommand(VkCommandBuffer commandBuffer) {
            const uint32_t LOCAL_GROUP_X = 32u, LOCAL_GROUP_Y = 24u;
            
        };

    };

};