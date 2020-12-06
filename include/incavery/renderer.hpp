#pragma once

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"
#include "./geometryLevel.hpp"
#include "./instanceLevel.hpp"

// 
namespace icv {

    struct RendererInfo
    {
        vkt::uni_ptr<GeometryRegistry> geometryRegistry = {};
        vkt::uni_ptr<InstanceLevel> instanceLevel = {};
    };

    struct FramebufferInfo
    {
        std::vector<vkt::ImageRegion> images = {};
        vkt::ImageRegion depthImage = {};

        //
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkDescriptorSet set = VK_NULL_HANDLE;
        bool created = false;

        // planned multiple viewports
        vkh::VkViewport viewport = {};
        vkh::VkRect2D scissor = {};

        //
        FramebufferInfo& transfer(vkt::uni_ptr<vkf::Queue> queue) {
            queue->submitOnce([&](VkCommandBuffer commandBuffer){
                for (auto& image : this->images) {
                    image.transfer(commandBuffer);
                };
                depthImage.transfer(commandBuffer);
            });
            return *this;
        };
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
        GraphicsPipelineSource opaque = {
            "./shaders/opaque.vert.spv",
            "./shaders/opaque.geom.spv",
            "./shaders/opaque.frag.spv"
        };
        GraphicsPipelineSource translucent = {
            "./shaders/translucent.vert.spv",
            "./shaders/translucent.geom.spv",
            "./shaders/translucent.frag.spv"
        };
    };

    struct PushConstantInfo 
    {
        uint32_t instanceId = 0u;
        uint32_t geometryId = 0u;
        uint32_t reserved0 = 0u;
        uint32_t reserved1 = 0u;
    };

    struct DrawInfo
    {
        uint32_t type = 0u;
        uint32_t reserved0 = 0u;
        PushConstantInfo constants = {};
    };

    struct DescriptorLayouts 
    {   // 
        VkDescriptorSetLayout framebuffer = VK_NULL_HANDLE;
        VkDescriptorSetLayout geometryRegistry = VK_NULL_HANDLE;
        VkDescriptorSetLayout instanceLevel = VK_NULL_HANDLE;
    };

    class Renderer: public DeviceBased
    {
        protected: 
        RendererInfo info = {};
        FramebufferInfo framebuffer = {};
        PipelineInfo pipeline = {};
        DescriptorLayouts layouts = {};

        // 
        VkRenderPass renderPass = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets = {};

        // 
        const uint32_t FBO_COUNT = 4u;

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
        virtual std::vector<VkDescriptorSet>& editDescriptorSets() 
        {
            return descriptorSets;
        };

        // 
        virtual const std::vector<VkDescriptorSet>& editDescriptorSets() const
        {
            return descriptorSets;
        };

        //
        virtual VkDescriptorSetLayout& getInstanceLevelLayout() 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            if (!layouts.instanceLevel) 
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
                vkh::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &layouts.instanceLevel));
            };

            return layouts.instanceLevel;
        };

        //
        virtual VkDescriptorSetLayout& getGeometryRegistryLayout() 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            if (!layouts.geometryRegistry) 
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
                vkh::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &layouts.geometryRegistry));
            };

            return layouts.geometryRegistry;
        };

        //
        virtual VkDescriptorSetLayout& getFramebufferLayout() 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            if (!layouts.framebuffer) 
            {   // create descriptor set layout
                vkh::VsDescriptorSetLayoutCreateInfoHelper descriptorSetLayoutHelper(vkh::VkDescriptorSetLayoutCreateInfo{});
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 0u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = FBO_COUNT,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1, .eVariableDescriptorCount = 1 });
                vkh::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &layouts.framebuffer));
            };

            return layouts.framebuffer;
        };


        //
        virtual const VkDescriptorSetLayout& getInstanceLevelLayout() const 
        {   // 
            return layouts.instanceLevel;
        };

        //
        virtual const VkDescriptorSetLayout& getGeometryRegistryLayout() const 
        {   // 
            return layouts.geometryRegistry;
        };

        //
        virtual const VkDescriptorSetLayout& getFramebufferLayout() const 
        {   // 
            return layouts.framebuffer;
        };


        //
        virtual VkPipelineLayout& createPipelineLayout(std::vector<VkDescriptorSetLayout> layouts = {}) 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            // 
            layouts.insert(layouts.begin(), this->getInstanceLevelLayout()); // 3th
            layouts.insert(layouts.begin(), this->getGeometryRegistryLayout()); // secondly
            layouts.insert(layouts.begin(), this->getFramebufferLayout()); // firstly

            // 
            std::vector<vkh::VkPushConstantRange> rtRanges = { vkh::VkPushConstantRange{.stageFlags = pipusage, .offset = 0u, .size = 16u } };
            vkh::handleVk(device->dispatch->CreatePipelineLayout(vkh::VkPipelineLayoutCreateInfo{  }.setSetLayouts(layouts).setPushConstantRanges(rtRanges), nullptr, &pipeline.layout));
            return pipeline.layout;
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
            if (!pipeline.layout) { this->createPipelineLayout(); };
            return pipeline.layout;
        };

        // 
        virtual FramebufferInfo& createFramebuffer(vkh::VkExtent3D size = {})
        {   //
            std::vector<VkImageView> views = {};

            for (uint32_t i=0;i<FBO_COUNT;i++) 
            {   // 
                framebuffer.images.push_back(createImage2D(ImageCreateInfo{.usage = VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .extent = size, .isDepth = false}));
                views.push_back(framebuffer.images.back());
            };

            {   // 
                framebuffer.depthImage = createImage2D(ImageCreateInfo{.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, .format = VK_FORMAT_D32_SFLOAT_S8_UINT, .extent = size, .isDepth = true});
                views.push_back(framebuffer.depthImage);
            };

            {   // 
                vkh::handleVk(device->dispatch->CreateFramebuffer(vkh::VkFramebufferCreateInfo{ .flags = {}, .renderPass = renderPass, .attachmentCount = uint32_t(views.size()), .pAttachments = views.data(), .width = size.width, .height = size.height, .layers = 1u }, nullptr, &framebuffer.framebuffer));
            };

            {   // 
                framebuffer.scissor = vkh::VkRect2D{ vkh::VkOffset2D{0, 0}, vkh::VkExtent2D{ size.width, size.height } };
                framebuffer.viewport = vkh::VkViewport{ 0.0f, 0.0f, static_cast<float>(size.width), static_cast<float>(size.height), 0.f, 1.f };
            };

            {   // create descriptor set
                vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(layouts.framebuffer, device->descriptorPool);
                auto handle = descriptorSetHelper.pushDescription<vkh::VkDescriptorImageInfo>(vkh::VkDescriptorUpdateTemplateEntry
                {
                    .dstBinding = 0u,
                    .descriptorCount = uint32_t(FBO_COUNT),
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                });
                for (uint32_t i=0;i<FBO_COUNT;i++) 
                {
                    handle[i] = framebuffer.images[i];
                };
                vkh::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, framebuffer.set, framebuffer.created);
            };

            return framebuffer;
        };

        // 
        virtual PipelineInfo& createPipeline(vkt::uni_arg<PipelineCreateInfo> info = PipelineCreateInfo{}) 
        {   // 
            pipeline.rayTracing = vkt::createCompute(device->dispatch, info->rayTracing.path, this->getPipelineLayout(), device->pipelineCache);

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
                pipelineInfo.depthStencilState = vkh::VkPipelineDepthStencilStateCreateInfo{ .depthTestEnable = true, .depthWriteEnable = true };
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
                vkh::handleVk(device->dispatch->CreateGraphicsPipelines(device->pipelineCache, 1u, pipelineInfo, nullptr, &pipeline.opaqueRasterization));
            };

            {   // translucent rasterization
                pipelineInfo.stages = {
                    vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info->translucent.vertex), VK_SHADER_STAGE_VERTEX_BIT),
                    vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info->translucent.geometry), VK_SHADER_STAGE_GEOMETRY_BIT),
                    vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(info->translucent.fragment), VK_SHADER_STAGE_FRAGMENT_BIT)
                };
                vkh::handleVk(device->dispatch->CreateGraphicsPipelines(device->pipelineCache, 1u, pipelineInfo, nullptr, &pipeline.translucentRasterization));
            };

            return pipeline;
        };

        //
        virtual std::vector<VkDescriptorSet>& makeDescriptorSets()
        {   //
            
            if (this->descriptorSets.size() < 3u) { this->descriptorSets.resize(3u); };
            this->descriptorSets[0u] = this->framebuffer.set;
            this->descriptorSets[1u] = this->info.geometryRegistry->makeDescriptorSet( DescriptorInfo{ .layout = layouts.geometryRegistry, .pipelineLayout = pipeline.layout } );
            this->descriptorSets[2u] = this->info.instanceLevel->makeDescriptorSet( DescriptorInfo{ .layout = layouts.instanceLevel, .pipelineLayout = pipeline.layout } );
            return this->descriptorSets;
        };

        // 
        virtual void createRenderingCommand(VkCommandBuffer commandBuffer) 
        {   //
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            //
            //auto descriptorSets = std::vector<VkDescriptorSet>();
            //descriptorSets.resize(3u);
            //for (auto& set : this->descriptorSets) {
            //    descriptorSets.push_back(set);
            //};

            // 
            const uint32_t LOCAL_GROUP_X = 32u, LOCAL_GROUP_Y = 24u;
            
            // draw items
            std::vector<DrawInfo> opaque = {};
            std::vector<DrawInfo> translucent = {};

            // select opaque and translucent for draw
            auto instanceLevelInfo = info.instanceLevel->getInfo();
            for (uint32_t I=0;I<instanceLevelInfo.instances.size();I++) {
                auto& instanceInfo = instanceLevelInfo.instances[I];
                auto& geometryLevel = instanceLevelInfo.geometries[instanceInfo.instanceCustomIndex];
                auto& geometryLevelInfo = geometryLevel->getInfo();
                for (uint32_t G=0;G<geometryLevelInfo.geometries.size();G++) {
                    auto& geometryInfo = geometryLevelInfo.geometries[G];
                    if (geometryInfo.isOpaque) {
                        opaque.push_back(DrawInfo
                        {
                            .type = 0u,
                            .constants = {I, G, 0u, 0u}
                        });
                    } else
                    {
                        translucent.push_back(DrawInfo
                        {
                            .type = 1u,
                            .constants = {I, G, 0u, 0u}
                        });
                    }
                };
            };

            // TODO: use with framebuffer as native
            std::vector<vkh::VkClearValue> clearValues = {};
            for (uint32_t i=0;i<FBO_COUNT;i++) {
                clearValues.push_back(vkh::VkClearColorValue{});
                clearValues.back().color.float32 = glm::vec4(0.f, 0.f, 0.f, 0.f);
            };
            clearValues.push_back(vkh::VkClearDepthStencilValue{ 1.0f, 0 });

            // shortify code by lambda function
            auto& geometryRegistryInfo = info.geometryRegistry->getInfo();
            auto drawItem = [&,this](DrawInfo& item){
                auto& instanceInfo = instanceLevelInfo.instances[item.constants.instanceId];
                auto& geometryLevel = instanceLevelInfo.geometries[instanceInfo.instanceCustomIndex];
                auto& geometryLevelInfo = geometryLevel->getInfo();
                auto& geometryInfo = geometryLevelInfo.geometries[item.constants.geometryId];

                // 
                auto& indexBuffer = geometryRegistryInfo.buffers[geometryInfo.index.buffer];
                auto& vertexBuffer = geometryRegistryInfo.buffers[geometryInfo.vertex.buffer];
                std::vector<VkBuffer> buffers = { vertexBuffer.buffer };
                std::vector<VkDeviceSize> offsets = { vertexBuffer.offset };
                std::vector<VkDeviceSize> ranges = { vertexBuffer.range };
                std::vector<VkDeviceSize> strides = { geometryInfo.vertex.stride };

                // 
                device->dispatch->CmdPushConstants(commandBuffer, pipeline.layout, pipusage, 0u, sizeof(PushConstantInfo), &item.constants);
                device->dispatch->CmdBindVertexBuffers2EXT(commandBuffer, 0u, buffers.size(), buffers.data(), offsets.data(), ranges.data(), strides.data());

                // rasterize command
                if (geometryInfo.index.type == 0u) {
                    device->dispatch->CmdDraw(commandBuffer, geometryInfo.primitive.count*3u, 1u, geometryInfo.index.first, 0u);
                } else {
                    
                    device->dispatch->CmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, indexBuffer.offset, getIndexType(geometryInfo.index.type));
                    device->dispatch->CmdDrawIndexed(commandBuffer, geometryInfo.primitive.count*3u, 1u, 0u, geometryInfo.index.first, 0u);
                };
            };

            {
                for (auto& image : framebuffer.images) {
                    device->dispatch->CmdClearColorImage(commandBuffer, image, image.getImageLayout(), vkh::VkClearColorValue{ .float32 = { 0.f,0.f,0.f,0.f } }, 1u, image.getImageSubresourceRange());
                };
                device->dispatch->CmdClearDepthStencilImage(commandBuffer, framebuffer.depthImage, framebuffer.depthImage.getImageLayout(), vkh::VkClearDepthStencilValue{ .depth = 1.0f, .stencil = 0 }, 1u, framebuffer.depthImage.getImageSubresourceRange());
                vkt::commandBarrier(device->dispatch, commandBuffer);
            };

            {
                device->dispatch->CmdBeginRenderPass(commandBuffer, vkh::VkRenderPassBeginInfo{ .renderPass = renderPass, .framebuffer = framebuffer.framebuffer, .renderArea = framebuffer.scissor, .clearValueCount = uint32_t(clearValues.size()), .pClearValues = reinterpret_cast<vkh::VkClearValue*>(clearValues.data()) }, VK_SUBPASS_CONTENTS_INLINE);

                {   // initial rasterization layout
                    device->dispatch->CmdSetViewport(commandBuffer, 0u, 1u, framebuffer.viewport);
                    device->dispatch->CmdSetScissor(commandBuffer, 0u, 1u, framebuffer.scissor);
                    device->dispatch->CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0u, descriptorSets.size(), descriptorSets.data(), 0u, nullptr);
                };

                {   // draw opaque
                    device->dispatch->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.opaqueRasterization);
                    for (auto& item : opaque) { drawItem(item); };
                    vkt::commandBarrier(device->dispatch, commandBuffer);

                    // draw translucent
                    device->dispatch->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.translucentRasterization);
                    for (auto& item : translucent) { drawItem(item); };
                    vkt::commandBarrier(device->dispatch, commandBuffer);
                };

                device->dispatch->CmdEndRenderPass(commandBuffer);
            };

            {   // compute ray tracing
                device->dispatch->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.rayTracing);
                device->dispatch->CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout, 0u, descriptorSets.size(), descriptorSets.data(), 0u, nullptr);
                device->dispatch->CmdDispatch(commandBuffer, framebuffer.scissor.extent.width/LOCAL_GROUP_X, framebuffer.scissor.extent.height/LOCAL_GROUP_Y, 1u);
                vkt::commandBarrier(device->dispatch, commandBuffer);
            };

        };

    };

};