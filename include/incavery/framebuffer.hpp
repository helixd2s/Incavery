#pragma once

// 
#include "./core.hpp"
#include "./geometryRegistry.hpp"
#include "./geometryLevel.hpp"
#include "./instanceLevel.hpp"

// 
namespace icv {

    struct FramebufferStateInfo
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
        FramebufferStateInfo& transfer(vkt::uni_ptr<vkf::Queue> queue) {
            queue->submitOnce([&](VkCommandBuffer commandBuffer){
                for (auto& image : this->images) {
                    image.transfer(commandBuffer);
                };
                depthImage.transfer(commandBuffer);
            });
            return *this;
        };
    };

    struct FramebufferInfo 
    {
        vkh::VkExtent3D size = {};
        VkDescriptorSetLayout layout = {};
        VkRenderPass renderPass = {};
    };


    class Framebuffer: public DeviceBased
    {
        protected: // 
        FramebufferStateInfo framebuffer = {};
        FramebufferInfo info = {};
        vkt::uni_ptr<vkf::Device> device = {};

        // 
        virtual void constructor(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<FramebufferInfo> info = FramebufferInfo{}) 
        {
            this->info = info;
            this->device = device;

        };

        public:
        Framebuffer() {};
        Framebuffer(vkt::uni_ptr<vkf::Device> device, vkt::uni_arg<FramebufferInfo> info = FramebufferInfo{}) { this->constructor(device, info); };

        //
        static const uint32_t FBO_COUNT = 4u;

        //
        virtual FramebufferStateInfo& getState() {
            return framebuffer;
        };

        //
        virtual const FramebufferStateInfo& getState() const {
            return framebuffer;
        };
        
        //
        static VkRenderPass& createRenderPass(vkt::uni_ptr<vkf::Device> device, VkRenderPass& renderPass) {
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
        static VkDescriptorSetLayout& createDescriptorSetLayout(vkt::uni_ptr<vkf::Device> device, VkDescriptorSetLayout& descriptorSetLayout) {
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            if (!descriptorSetLayout) 
            {   // create descriptor set layout
                vkh::VsDescriptorSetLayoutCreateInfoHelper descriptorSetLayoutHelper(vkh::VkDescriptorSetLayoutCreateInfo{});
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 0u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = FBO_COUNT,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1 });
                vkh::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &descriptorSetLayout));
            };

            return descriptorSetLayout;
        };

        // 
        virtual FramebufferStateInfo& createFramebuffer(vkt::uni_ptr<vkf::Queue> queue = {})
        {   //
            std::vector<VkImageView> views = {};
            std::vector<VkFramebufferAttachmentImageInfo> attachments = {};

            // 
            vkh::VkSamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.unnormalizedCoordinates = false;

            // 
            VkSampler sampler = VK_NULL_HANDLE;
            vkt::handle(device->dispatch->CreateSampler(samplerCreateInfo, nullptr, &sampler));
            
            // 
            for (uint32_t i=0;i<FBO_COUNT;i++) 
            {   // 
                framebuffer.images.push_back(createImage2D(ImageCreateInfo{.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .extent = info.size, .isDepth = false}));
                framebuffer.images.back().getDescriptor().sampler = sampler;
                views.push_back(framebuffer.images.back());
                attachments.push_back(VkFramebufferAttachmentImageInfo{
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
                    .pNext = nullptr, 
                    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    .width = info.size.width,
                    .height = info.size.height,
                    .layerCount = 1u,
                    .viewFormatCount = 1u,
                    .pViewFormats = &framebuffer.images.back().getInfo().format
                });
            };

            {   // 
                framebuffer.depthImage = createImage2D(ImageCreateInfo{.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, .format = VK_FORMAT_D32_SFLOAT_S8_UINT, .extent = info.size, .isDepth = true});
                views.push_back(framebuffer.depthImage);
                attachments.push_back(VkFramebufferAttachmentImageInfo{
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
                    .pNext = nullptr, 
                    .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    .width = info.size.width,
                    .height = info.size.height,
                    .layerCount = 1u,
                    .viewFormatCount = 1u,
                    .pViewFormats = &framebuffer.depthImage.getInfo().format
                });
            };

            {   // TODO: NVIDIA doesn't support imageless framebuffer (stub-only)
                VkFramebufferAttachmentsCreateInfo attachmentInfo = {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO,
                    .pNext = nullptr,
                    .attachmentImageInfoCount = uint32_t(attachments.size()),
                    .pAttachmentImageInfos = attachments.data()
                };

                if (queue) { framebuffer.transfer(queue); };
                vkh::handleVk(device->dispatch->CreateFramebuffer(vkh::VkFramebufferCreateInfo{ .pNext = &attachmentInfo, .flags = {}, .renderPass = info.renderPass, .attachmentCount = uint32_t(views.size()), .pAttachments = views.data(), .width = info.size.width, .height = info.size.height, .layers = 1u }, nullptr, &framebuffer.framebuffer));
            };

            {   // 
                framebuffer.scissor = vkh::VkRect2D{ vkh::VkOffset2D{0, 0}, vkh::VkExtent2D{ info.size.width, info.size.height } };
                framebuffer.viewport = vkh::VkViewport{ 0.0f, 0.0f, static_cast<float>(info.size.width), static_cast<float>(info.size.height), 0.f, 1.f };
            };
            
            {   // create descriptor set
                vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(info.layout, device->descriptorPool);
                auto handle = descriptorSetHelper.pushDescription<vkh::VkDescriptorImageInfo>(vkh::VkDescriptorUpdateTemplateEntry
                {
                    .dstBinding = 0u,
                    .descriptorCount = uint32_t(FBO_COUNT),
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                });
                for (uint32_t i=0;i<FBO_COUNT;i++) 
                {
                    handle[i] = framebuffer.images[i];
                };
                vkh::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, framebuffer.set, framebuffer.created);
            };

            return framebuffer;
        };
    };

};
