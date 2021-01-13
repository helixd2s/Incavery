#define VMA_IMPLEMENTATION

//
#include <vkf/swapchain.hpp>
#include <glm/gtc/matrix_transform.hpp>

//
#include <incavery/core.hpp>
#include <incavery/dataSet.hpp>
#include <incavery/geometryRegistry.hpp>
#include <incavery/geometryLevel.hpp>
#include <incavery/instanceLevel.hpp>
#include <incavery/renderer.hpp>

//
#include <FreeImage.h>

// 
void error(int errnum, const char* errmsg)
{
    std::cerr << errnum << ": " << errmsg << std::endl;
};

// 
const uint32_t SCR_WIDTH = 640u, SCR_HEIGHT = 360u;

//
struct Constants
{
    glm::mat4x4 perspective = glm::mat4x4(1.f);
    glm::mat4x4 perspectiveInverse = glm::mat4x4(1.f);
    glm::mat3x4 lookAt = glm::mat3x4(1.f);
    glm::mat3x4 lookAtInverse = glm::mat3x4(1.f);
};

// 
int main() {
    glfwSetErrorCallback(error);
    glfwInit();

    // 
    if (GLFW_FALSE == glfwVulkanSupported()) {
        glfwTerminate(); return -1;
    };

    // 
    uint32_t canvasWidth = SCR_WIDTH, canvasHeight = SCR_HEIGHT;
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // 
    float xscale = 1.f, yscale = 1.f;
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    glfwGetMonitorContentScale(primary, &xscale, &yscale);

    // 
    std::string title = "TestApp";
    vkh::uni_ptr<vkf::Instance> instance = std::make_shared<vkf::Instance>();
    vkh::uni_ptr<vkf::Device> device = std::make_shared<vkf::Device>(instance);
    vkh::uni_ptr<vkf::Queue> queue = std::make_shared<vkf::Queue>(device);
    vkh::uni_ptr<vkf::SwapChain> manager = std::make_shared<vkf::SwapChain>(device);

    // 
    instance->create();
    vkf::SurfaceWindow& surface = manager->createWindowSurface(SCR_WIDTH * xscale, SCR_HEIGHT * yscale, title);

    // 
    device->create(0u, surface.surface);
    queue->create();

    // 
    vkf::SurfaceFormat& format = manager->getSurfaceFormat();
    VkRenderPass& renderPass = manager->createRenderPass();
    VkSwapchainKHR& swapchain = manager->createSwapchain();
    std::vector<vkf::Framebuffer>& framebuffers = manager->createSwapchainFramebuffer(queue);

    // 
    auto allocInfo = vkf::MemoryAllocationInfo{};
    allocInfo.device = *device;
    allocInfo.memoryProperties = device->memoryProperties;
    allocInfo.instanceDispatch = instance->dispatch;
    allocInfo.deviceDispatch = device->dispatch;

    // 
    auto renderArea = vkh::VkRect2D{ vkh::VkOffset2D{0, 0}, vkh::VkExtent2D{ uint32_t(canvasWidth * xscale), uint32_t(canvasHeight * yscale) } };
    auto downscaled = vkh::VkExtent2D{ uint32_t(canvasWidth * 2.f), uint32_t(canvasHeight * 2.f) };
    auto viewport = vkh::VkViewport{ 0.0f, 0.0f, static_cast<float>(renderArea.extent.width), static_cast<float>(renderArea.extent.height), 0.f, 1.f };
    


    //
    Constants constants = {};

    //
    std::vector<uint16_t> indices = { 0, 1, 2 };
    std::vector<glm::vec4> vertices = {
        glm::vec4(1.f, -1.f, 1.f, 1.f),
        glm::vec4(-1.f, -1.f, 1.f, 1.f),
        glm::vec4(0.f,  1.f, 1.f, 1.f)
    };
    std::vector<glm::vec2> texcoords = {
        glm::vec2(1.f, 0.f),
        glm::vec2(0.f, 0.f),
        glm::vec2(0.5f, 1.f)
    };
    std::vector<uint32_t> primitiveCounts = { 1u };
    std::vector<uint32_t> instanceCounts = { 1u };

    //
    vkf::Vector<uint16_t> indicesBuffer = {};
    vkf::Vector<glm::vec4> verticesBuffer = {};
    vkf::Vector<glm::vec2> texcoordsBuffer = {};
    vkf::Vector<Constants> constantsBuffer = {};

    {   // vertices
        auto size = vertices.size() * sizeof(glm::vec4);
        auto bufferCreateInfo = vkh::VkBufferCreateInfo{
            .size = size,
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        };
        auto vmaCreateInfo = vkf::VmaMemoryInfo{
            .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
            .instanceDispatch = instance->dispatch,
            .deviceDispatch = device->dispatch
        };
        auto allocation = std::make_shared<vkf::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
        verticesBuffer = vkf::Vector<glm::vec4>(allocation, 0ull, size, sizeof(glm::vec4));
        //memcpy(verticesBuffer.map(), vertices.data(), size);
        queue->uploadIntoBuffer(verticesBuffer, vertices.data(), size); // use internal cache for upload buffer
    };

    {   // texcoords
        auto size = texcoords.size() * sizeof(glm::vec2);
        auto bufferCreateInfo = vkh::VkBufferCreateInfo{
            .size = size,
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        };
        auto vmaCreateInfo = vkf::VmaMemoryInfo{
            .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
            .instanceDispatch = instance->dispatch,
            .deviceDispatch = device->dispatch
        };
        auto allocation = std::make_shared<vkf::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
        texcoordsBuffer = vkf::Vector<glm::vec2>(allocation, 0ull, size, sizeof(glm::vec2));
        //memcpy(verticesBuffer.map(), vertices.data(), size);
        queue->uploadIntoBuffer(texcoordsBuffer, texcoords.data(), size); // use internal cache for upload buffer
    };

    {   // indices
        auto size = indices.size() * sizeof(uint16_t);
        auto bufferCreateInfo = vkh::VkBufferCreateInfo{
            .size = size,
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        };
        auto vmaCreateInfo = vkf::VmaMemoryInfo{
            .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
            .instanceDispatch = instance->dispatch,
            .deviceDispatch = device->dispatch
        };
        auto allocation = std::make_shared<vkf::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
        indicesBuffer = vkf::Vector<uint16_t>(allocation, 0ull, size, sizeof(uint16_t));
        //memcpy(indicesBuffer.map(), indices.data(), size);
        queue->uploadIntoBuffer(indicesBuffer, indices.data(), size); // use internal cache for upload buffer
    };

    {   // constants (with direct host access)
        auto size = sizeof(Constants);
        auto bufferCreateInfo = vkh::VkBufferCreateInfo{
            .size = size,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        };
        auto vmaCreateInfo = vkf::VmaMemoryInfo{
            .memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            .instanceDispatch = instance->dispatch,
            .deviceDispatch = device->dispatch
        };
        auto allocation = std::make_shared<vkf::VmaBufferAllocation>(device->allocator, bufferCreateInfo, vmaCreateInfo);
        constantsBuffer = vkf::Vector<Constants>(allocation, 0ull, size, sizeof(Constants));
        memcpy(constantsBuffer.map(), &constants, size);
        //queue->uploadIntoBuffer(constantsBuffer, &constants, size); // use internal cache for upload buffer
    };





    vkf::ImageRegion image = {};
    {
        // 
        vkh::VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        imageCreateInfo.extent = vkh::VkExtent3D{ downscaled.width, downscaled.height, 1 };
        imageCreateInfo.mipLevels = 1u;
        imageCreateInfo.arrayLayers = 1u;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // TODO: Linear
        imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // 
        auto vmaCreateInfo = vkf::VmaMemoryInfo{
            .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
            .instanceDispatch = instance->dispatch,
            .deviceDispatch = device->dispatch
        };

        // 
        vkh::VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        imageViewCreateInfo.components = vkh::VkComponentMapping{};
        imageViewCreateInfo.subresourceRange = vkh::VkImageSubresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0u, .levelCount = 1u, .baseArrayLayer = 0u, .layerCount = 1u };

        // 
        auto allocation = std::make_shared<vkf::VmaImageAllocation>(device->allocator, imageCreateInfo, vmaCreateInfo);
        image = vkf::ImageRegion(allocation, imageViewCreateInfo);

        // transfer image
        queue->submitOnce([&](VkCommandBuffer cmd) {
            image.transfer(cmd);
        });
    };



    //
    auto pipusage = vkh::VkShaderStageFlags{ .eVertex = 1, .eGeometry = 1, .eFragment = 1, .eCompute = 1, .eRaygen = 1, .eAnyHit = 1, .eClosestHit = 1, .eMiss = 1 };
    auto indexedf = vkh::VkDescriptorBindingFlags{ .eUpdateAfterBind = 1, .eUpdateUnusedWhilePending = 1, .ePartiallyBound = 1 };
    auto dflags = vkh::VkDescriptorSetLayoutCreateFlags{ .eUpdateAfterBindPool = 1 };

    //
    VkDescriptorSetLayout constantsLayout = VK_NULL_HANDLE;
    VkDescriptorSet constantsSet = VK_NULL_HANDLE;

    // create descriptor set layout
    vkh::VsDescriptorSetLayoutCreateInfoHelper descriptorSetLayoutHelper(vkh::VkDescriptorSetLayoutCreateInfo{});
    descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
        .binding = 0u,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1u, // TODO: fix descriptor counting
        .stageFlags = pipusage
    }, vkh::VkDescriptorBindingFlags{});
    descriptorSetLayoutHelper.pushBinding(vkh::VkDescriptorSetLayoutBinding{
        .binding = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1u, // TODO: fix descriptor counting
        .stageFlags = pipusage
    }, vkh::VkDescriptorBindingFlags{});
    vkt::handleVk(device->dispatch->CreateDescriptorSetLayout(descriptorSetLayoutHelper.format(), nullptr, &constantsLayout));



    // create descriptor set
    vkh::VsDescriptorSetCreateInfoHelper descriptorSetHelper(constantsLayout, device->descriptorPool);
    descriptorSetHelper.pushDescription<vkh::VkDescriptorBufferInfo>(vkh::VkDescriptorUpdateTemplateEntry{
        .dstBinding = 0u,
        .descriptorCount = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    }) = constantsBuffer;
    descriptorSetHelper.pushDescription<vkh::VkDescriptorImageInfo>(vkh::VkDescriptorUpdateTemplateEntry{
        .dstBinding = 1u,
        .descriptorCount = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
    }) = image;

    bool created = false;
    vkt::AllocateDescriptorSetWithUpdate(device->dispatch, descriptorSetHelper, constantsSet, created);


    // TODO: needs flush data
    vkh::uni_ptr<icv::MaterialSet> materialSet = std::make_shared<icv::MaterialSet>(device, icv::MaterialSetInfo{
        .maxMaterialCount = 8u
    });

    //
    vkh::uni_ptr<icv::GeometryRegistry> geometryRegistry = std::make_shared<icv::GeometryRegistry>(device, icv::GeometryRegistryInfo{
        .maxBindingCount = 8u
    });

    //
    vkh::uni_ptr<icv::GeometryLevel> geometryLevel = std::make_shared<icv::GeometryLevel>(device, icv::GeometryLevelInfo{
        .registry = geometryRegistry,
        .maxGeometryCount = 1u
    });

    // 
    vkh::uni_ptr<icv::InstanceLevel> instanceLevel = std::make_shared<icv::InstanceLevel>(device, icv::InstanceLevelInfo{
        .registry = geometryRegistry,
        .maxInstanceCount = 1u
    });

    // 
    vkh::uni_ptr<icv::Renderer> renderer = std::make_shared<icv::Renderer>(device, icv::RendererInfo{
        .geometryRegistry = geometryRegistry, 
        .instanceLevel = instanceLevel,
        .materialSet = materialSet
    });

    //
    vkh::uni_ptr<icv::Framebuffer> framebuffer = std::make_shared<icv::Framebuffer>(device, icv::FramebufferInfo{
        .size = { downscaled.width, downscaled.height, 1u },
        .layout = renderer->getFramebufferLayout(),
        .renderPass = renderer->createRenderPass()
    });

    //
    geometryLevel->pushGeometry(icv::GeometryInfo{
        .vertex = {
            .stride = sizeof(glm::vec4),
            .ptr = verticesBuffer.deviceAddress() //{ .bufferId = 1u }
        },
        .index = {
            .max = 3u,
            .type = 2u,
            .ptr = indicesBuffer.deviceAddress() //{ .bufferId = 0u }
        },
        .primitive = {
            .count = 1u
        },
        .attributes = {
            .texcoords = 2u
        }
    });

    // push buffers into registry
    geometryRegistry->pushBuffer(indicesBuffer);
    geometryRegistry->pushBuffer(verticesBuffer);
    geometryRegistry->pushBuffer(texcoordsBuffer);

    // separate buffer and binding push
    geometryRegistry->pushBinding(icv::BindingInfo{
        .format = 0u,
        .stride = 2u,
        .ptr = indicesBuffer.deviceAddress() //{ .bufferId = 0u }
    });

    geometryRegistry->pushBinding(icv::BindingInfo{
        .format = 0u,
        .stride = sizeof(glm::vec4),
        .ptr = verticesBuffer.deviceAddress() //{ .bufferId = 1u }
    });

    geometryRegistry->pushBinding(icv::BindingInfo{
        .format = 0u,
        .stride = sizeof(glm::vec2),
        .ptr = texcoordsBuffer.deviceAddress() //{ .bufferId = 2u }
    });

    //
    instanceLevel->pushInstance(icv::InstanceInfo{
        .instanceCustomIndex = uint32_t(instanceLevel->pushGeometryLevel(geometryLevel))
    });


    // 
    FREE_IMAGE_FORMAT formato = FreeImage_GetFileType("sample.png", 0);
    if (formato == FIF_UNKNOWN) { /* go angry here */ }
    FIBITMAP* imagen = FreeImage_Load(formato, "sample.png");
    if (!imagen) { /* go very angry here */ }
    FIBITMAP* temp = FreeImage_ConvertTo32Bits(imagen);
    if (!imagen) { /* go mad here */ }
    FreeImage_Unload(imagen);
    imagen = temp;

    // 
    vkf::ImageRegion texture = {};
    {
        int width = FreeImage_GetWidth(imagen);
        int height = FreeImage_GetHeight(imagen);
        char* pixeles = (char*)FreeImage_GetBits(imagen);

        // 
        vkh::VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        imageCreateInfo.extent = vkh::VkExtent3D{ uint32_t(width), uint32_t(height), 1 };
        imageCreateInfo.mipLevels = 1u;
        imageCreateInfo.arrayLayers = 1u;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // TODO: Linear
        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // 
        auto vmaCreateInfo = vkf::VmaMemoryInfo{
            .memUsage = VMA_MEMORY_USAGE_GPU_ONLY,
            .instanceDispatch = instance->dispatch,
            .deviceDispatch = device->dispatch
        };

        // 
        vkh::VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        imageViewCreateInfo.components = vkh::VkComponentMapping{};
        imageViewCreateInfo.subresourceRange = vkh::VkImageSubresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0u, .levelCount = 1u, .baseArrayLayer = 0u, .layerCount = 1u };

        // 
        auto allocation = std::make_shared<vkf::VmaImageAllocation>(device->allocator, imageCreateInfo, vmaCreateInfo);
        texture = vkf::ImageRegion(allocation, imageViewCreateInfo);

        // transfer image
        queue->submitOnce([&](VkCommandBuffer cmd) {
            texture.transfer(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        });

        // upload image
        queue->uploadIntoImage(texture, pixeles, vkh::VkOffset3D{0u,0u,0u}, imageCreateInfo.extent, texture.subresourceLayers());

        // transfer image
        queue->submitOnce([&](VkCommandBuffer cmd) {
            texture.transfer(cmd, VK_IMAGE_LAYOUT_GENERAL);
        });
    };

    // 
    VkSampler sampler = VK_NULL_HANDLE;
    {   // 
        vkh::VkSamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.unnormalizedCoordinates = false;

        // 
        vkt::handleVk(device->dispatch->CreateSampler(samplerCreateInfo, nullptr, &sampler));
    };

    //
    vkh::VkDescriptorImageInfo& textureDescriptor = texture.getDescriptor();
    textureDescriptor.sampler = sampler;


    // 
    materialSet->pushTexture(textureDescriptor);
    materialSet->pushMaterial(icv::MaterialSource{
        .baseColorFactor = {1.f,1.f,1.f,1.f},
        .baseColorTexture = 0
    });


    // create renderer
    framebuffer->createFramebuffer(queue);
    renderer->setFramebuffer(framebuffer);
    renderer->createPipelineLayout({ constantsLayout });
    renderer->createPipeline(icv::PipelineCreateInfo{});
    renderer->makeDescriptorSets();

    // 
    auto& descriptorSets = renderer->editDescriptorSets();
    auto& pipelineLayout = renderer->getPipelineLayout();

    // additional descriptor set
    descriptorSets.push_back(constantsSet);





    // graphics pipeline
    vkh::VsGraphicsPipelineCreateInfoConstruction pipelineInfo = {};
    pipelineInfo.stages = {
        vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(std::string("./shaders/render.vert.spv")), VK_SHADER_STAGE_VERTEX_BIT),
        vkt::makePipelineStageInfo(device->dispatch, vkt::readBinary(std::string("./shaders/render.frag.spv")), VK_SHADER_STAGE_FRAGMENT_BIT)
    };
    pipelineInfo.graphicsPipelineCreateInfo.layout = pipelineLayout;
    pipelineInfo.graphicsPipelineCreateInfo.renderPass = renderPass;//context->refRenderPass();
    pipelineInfo.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pipelineInfo.viewportState.pViewports = &reinterpret_cast<::VkViewport&>(viewport);
    pipelineInfo.viewportState.pScissors = &reinterpret_cast<::VkRect2D&>(renderArea);
    pipelineInfo.colorBlendAttachmentStates = { {} }; // Default Blend State
    pipelineInfo.dynamicStates = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };

    // 
    VkPipeline finalPipeline = {};
    vkt::handleVk(device->dispatch->CreateGraphicsPipelines(device->pipelineCache, 1u, pipelineInfo, nullptr, &finalPipeline));


    // set perspective
    auto persp = glm::perspective(60.f / 180 * glm::pi<float>(), viewport.width / viewport.height, 0.001f, 10000.f);
    auto lkat = glm::lookAt(glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f));
    constants.perspective = glm::transpose(persp);
    constants.perspectiveInverse = glm::transpose(glm::inverse(persp));
    constants.lookAt = glm::mat3x4(glm::transpose(lkat));
    constants.lookAtInverse = glm::mat3x4(glm::transpose(glm::inverse(lkat)));

    // 
    int64_t currSemaphore = -1;
    uint32_t currentBuffer = 0u;
    uint32_t frameCount = 0u;

    // 
    while (!glfwWindowShouldClose(surface.window)) { // 
        glfwPollEvents();

        // 
        int64_t n_semaphore = currSemaphore, c_semaphore = (currSemaphore + 1) % framebuffers.size(); // Next Semaphore
        currSemaphore = (c_semaphore = c_semaphore >= 0 ? c_semaphore : int64_t(framebuffers.size()) + c_semaphore); // Current Semaphore
        (n_semaphore = n_semaphore >= 0 ? n_semaphore : int64_t(framebuffers.size()) + n_semaphore); // Fix for Next Semaphores

        // 
        vkt::handleVk(device->dispatch->WaitForFences(1u, &framebuffers[c_semaphore].waitFence, true, 30ull * 1000ull * 1000ull * 1000ull));
        vkt::handleVk(device->dispatch->ResetFences(1u, &framebuffers[c_semaphore].waitFence));
        vkt::handleVk(device->dispatch->AcquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>::max(), framebuffers[c_semaphore].presentSemaphore, nullptr, &currentBuffer));
        //fw->getDeviceDispatch()->SignalSemaphore(vkh::VkSemaphoreSignalInfo{.semaphore = framebuffers[n_semaphore].semaphore, .value = 1u});

        // 
        vkh::VkClearValue clearValues[2] = { {}, {} };
        clearValues[0].color = vkh::VkClearColorValue{}; clearValues[0].color.float32 = glm::vec4(0.f, 0.f, 0.f, 0.f);
        clearValues[1].depthStencil = VkClearDepthStencilValue{ 1.0f, 0 };

        // Create render submission 
        std::vector<VkSemaphore> waitSemaphores = { framebuffers[currentBuffer].presentSemaphore }, signalSemaphores = { framebuffers[currentBuffer].drawSemaphore };
        std::vector<VkPipelineStageFlags> waitStages = {
            vkh::VkPipelineStageFlags{.eFragmentShader = 1, .eComputeShader = 1, .eTransfer = 1, .eRayTracingShader = 1, .eAccelerationStructureBuild = 1 },
            vkh::VkPipelineStageFlags{.eFragmentShader = 1, .eComputeShader = 1, .eTransfer = 1, .eRayTracingShader = 1, .eAccelerationStructureBuild = 1 }
        };

        // create command buffer (with rewrite)
        VkCommandBuffer& commandBuffer = framebuffers[currentBuffer].commandBuffer;
        if (!commandBuffer) {
            commandBuffer = vkt::createCommandBuffer(device->dispatch, queue->commandPool, false, false); // do reference of cmd buffer

            {   // Use as present image
                auto aspect = vkh::VkImageAspectFlags{ .eColor = 1u };
                vkt::imageBarrier(commandBuffer, vkt::ImageBarrierInfo{
                    .image = framebuffers[currentBuffer].image,
                    .targetLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .originLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .subresourceRange = vkh::VkImageSubresourceRange{ aspect, 0u, 1u, 0u, 1u }
                });
            };

            {   // Reuse depth as general
                auto aspect = vkh::VkImageAspectFlags{ .eDepth = 1u, .eStencil = 1u };
                vkt::imageBarrier(commandBuffer, vkt::ImageBarrierInfo{
                    .image = manager->depthImage.getImage(),
                    .targetLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .originLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .subresourceRange = vkh::VkImageSubresourceRange{ aspect, 0u, 1u, 0u, 1u }
                });
            };

            // ray tracing
            //device->dispatch->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipeline);
            //device->dispatch->CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0u, descriptorSets.size(), descriptorSets.data(), 0u, nullptr);
            //device->dispatch->CmdTraceRaysKHR(commandBuffer, &rayGenSbt, &missSbt, &hitSbt, nullptr, 1280, 720, 1);
            //vkt::commandBarrier(device->dispatch, commandBuffer);

            // ray query hack
            //device->dispatch->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, rayQueryPipeline);
            //device->dispatch->CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0u, descriptorSets.size(), descriptorSets.data(), 0u, nullptr);
            //device->dispatch->CmdDispatch(commandBuffer, 1280u/32u, 720u/4u, 1u);

            // 
            memcpy(constantsBuffer.map(), &constants, sizeof(Constants));
            vkt::commandBarrier(device->dispatch, commandBuffer);

            // 
            materialSet->copyCommand(commandBuffer);
            geometryRegistry->copyCommand(commandBuffer);
            geometryLevel->buildCommand(commandBuffer);
            instanceLevel->buildCommand(commandBuffer);
            renderer->createRenderingCommand(commandBuffer);
            vkt::commandBarrier(device->dispatch, commandBuffer);

            // rasterization
            device->dispatch->CmdBeginRenderPass(commandBuffer, vkh::VkRenderPassBeginInfo{ .renderPass = renderPass, .framebuffer = framebuffers[currentBuffer].frameBuffer, .renderArea = renderArea, .clearValueCount = 2u, .pClearValues = reinterpret_cast<vkh::VkClearValue*>(&clearValues[0]) }, VK_SUBPASS_CONTENTS_INLINE);
            device->dispatch->CmdSetViewport(commandBuffer, 0u, 1u, viewport);
            device->dispatch->CmdSetScissor(commandBuffer, 0u, 1u, renderArea);
            device->dispatch->CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline);
            device->dispatch->CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0u, descriptorSets.size(), descriptorSets.data(), 0u, nullptr);
            device->dispatch->CmdDraw(commandBuffer, 4, 1, 0, 0);
            device->dispatch->CmdEndRenderPass(commandBuffer);
            vkt::commandBarrier(device->dispatch, commandBuffer);

            // Use as present image
            {
                auto aspect = vkh::VkImageAspectFlags{ .eColor = 1u };
                vkt::imageBarrier(commandBuffer, vkt::ImageBarrierInfo{
                    .image = framebuffers[currentBuffer].image,
                    .targetLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .originLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .subresourceRange = vkh::VkImageSubresourceRange{ aspect, 0u, 1u, 0u, 1u }
                });
            };

            // Reuse depth as general
            {
                auto aspect = vkh::VkImageAspectFlags{ .eDepth = 1u, .eStencil = 1u };
                vkt::imageBarrier(commandBuffer, vkt::ImageBarrierInfo{
                    .image = manager->depthImage.getImage(),
                    .targetLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .originLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    .subresourceRange = vkh::VkImageSubresourceRange{ aspect, 0u, 1u, 0u, 1u }
                });
            };

            // 
            device->dispatch->EndCommandBuffer(commandBuffer);
        };

        // Submit command once
        vkt::handleVk(device->dispatch->QueueSubmit(queue->queue, 1u, vkh::VkSubmitInfo{
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()), .pWaitSemaphores = waitSemaphores.data(), .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = 1u, .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()), .pSignalSemaphores = signalSemaphores.data()
        }, framebuffers[currentBuffer].waitFence));

        // 
        waitSemaphores = { framebuffers[c_semaphore].drawSemaphore };
        vkt::handleVk(device->dispatch->QueuePresentKHR(queue->queue, vkh::VkPresentInfoKHR{
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()), .pWaitSemaphores = waitSemaphores.data(),
            .swapchainCount = 1, .pSwapchains = &swapchain,
            .pImageIndices = &currentBuffer, .pResults = nullptr
        }));



    };


    return 0;
};
