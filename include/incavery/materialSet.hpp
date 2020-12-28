#pragma once

// 
#include "./core.hpp"
#include "./dataSet.hpp"

// 
namespace icv {

    //
    struct MaterialSource
    {
        // factors
        glm::vec4 baseColorFactor;
        float metallicFactor = 0.f; // KHR
        float roughnessFactor = 0.f; // KHR
        float transmissionFactor = 0.f; // KHR
        float adobeIor = 1.f; // Adobe

        // textures
        int baseColorTexture = -1;
        int metallicRoughnessTexture = -1;
        int transmissionTexture = -1;
        int normalTexture = -1;
    };

    //
    struct MaterialSetInfo {
        std::vector<MaterialSource> materials = {};
        std::vector<vkh::VkDescriptorImageInfo> textures = {};

        uint32_t maxMaterialCount = 128u;
    };

    //
    class MaterialSet : public DeviceBased 
    {
        protected: 
        MaterialSetInfo info = {};
        vkh::uni_ptr<DataSet<MaterialSource>> materials = {};
        VkDescriptorSet set = VK_NULL_HANDLE;
        bool created = false;

        // 
        virtual void constructor(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<MaterialSetInfo> info = MaterialSetInfo{}) 
        {
            this->info = info;
            this->device = device;
            this->materials = std::make_shared<DataSet<MaterialSource>>(device, DataSetInfo{
                .count = info->maxMaterialCount,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            });
        };

        // 
        public:
        MaterialSet() {};
        MaterialSet(vkh::uni_ptr<vkf::Device> device, vkh::uni_arg<MaterialSetInfo> info = MaterialSetInfo{}) { this->constructor(device, info); };

        //
        static VkDescriptorSetLayout& createDescriptorSetLayout(vkh::uni_ptr<vkf::Device> device, VkDescriptorSetLayout& descriptorSetLayout) {
            auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
            auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
            auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

            if (!descriptorSetLayout) 
            {   // create descriptor set layout
                vkh::VsDescriptorSetLayoutCreateInfoHelper descriptorSetLayoutHelper(vkh::VkDescriptorSetLayoutCreateInfo{});
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 0u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 256u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1 });
                descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
                    .binding = 1u,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1u,
                    .stageFlags = pipusage
                }, vkh::VkDescriptorBindingFlags{ .ePartiallyBound = 1 });
                vkt::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &descriptorSetLayout));
            };

            return descriptorSetLayout;
        };

        //
        virtual const VkDescriptorSet& getDescriptorSet() const {
            return set;
        };

        //
        virtual VkDescriptorSet& getDescriptorSet() {
            return set;
        };

        //
        virtual const MaterialSetInfo& getInfo() const {
            return info;
        };

        //
        virtual MaterialSetInfo& getInfo() {
            return info;
        };

        //
        virtual const vkf::Vector<MaterialSource>& getBuffer() const {
            return materials->getDeviceBuffer();
        };

        //
        virtual vkf::Vector<MaterialSource>& getBuffer() {
            return materials->getDeviceBuffer();
        };

        // 
        virtual VkDescriptorSet& makeDescriptorSet(vkh::uni_arg<DescriptorInfo> info = DescriptorInfo{}) 
        {
            vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(info->layout, device->descriptorPool);
            if (this->info.textures.size() > 0) {
                auto handle = descriptorSetHelper.pushDescription<vkh::VkDescriptorImageInfo>(vkh::VkDescriptorUpdateTemplateEntry{
                    .dstBinding = 0u,
                    .descriptorCount = uint32_t(this->info.textures.size()),
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                    });
                memcpy(&handle, this->info.textures.data(), this->info.textures.size() * sizeof(vkh::VkDescriptorImageInfo));
            };
            descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry{
                .dstBinding = 1u,
                .descriptorCount = 1u,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
            }) = materials->getDeviceBuffer();
            vkt::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, set, created);
            return set;
        };

        //
        virtual void copyCommand(VkCommandBuffer commandBuffer)
        {   // 
            materials->copyFromVector(info.materials);
            materials->cmdCopyFromCpu(commandBuffer);
        };

        //
        virtual void flush(vkh::uni_ptr<vkf::Queue> queue = {}) 
        {   // 
            queue->submitOnce([&,this](VkCommandBuffer commandBuffer){
                this->copyCommand(commandBuffer);
            });
        };

        //
        virtual uintptr_t pushTexture(vkh::VkDescriptorImageInfo texture) 
        {   
            uintptr_t index = this->info.textures.size();
            this->info.textures.push_back(texture);
            return index;
        };

        //
        virtual uintptr_t pushMaterial(vkh::uni_arg<MaterialSource> material) 
        {   
            uintptr_t index = this->info.materials.size();
            this->info.materials.push_back(material);
            return index;
        };

        //
        void setMaterial(uintptr_t index, vkh::uni_arg<MaterialSource> material) {
            if (info.materials.size() <= index) { info.materials.resize(index+1u); };
            info.materials[index] = material;
        };

    };

};
