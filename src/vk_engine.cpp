#include <vulkan/vk_config.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vulkan/vk_engine.hpp>
#include <vulkan/vk_device.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <tiny_gltf_v3.h>
#include <ktx.h>
#include <ktxvulkan.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <random>
#include <ranges>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include <logs/logger.hpp>


struct UniformBufferObject{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector<char const*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

void VulkanEngine::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VulkanEngine::initWindow(){
    bool sdl_init = SDL_Init(SDL_INIT_VIDEO);           // Initialize SDL3
    if (!sdl_init) {SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not init sdl: %s\n", SDL_GetError());}

    window = SDL_CreateWindow("VulkanLearning", WIDTH, HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    if (window == NULL) {SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());}
}

void VulkanEngine::initVulkan() 
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    chooseMsaaCount();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createColorResources();
    createDepthResources();
    createGraphicsDescriptorSetLayout();
    createComputeDescriptorSetLayout(); // Compute
    createGraphicsPipeline();
    createComputePipeline(); // Compute
    createParticlePipeline(); // Compute's Graphics Pipeline
    createCommandPool();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    setupGameObjects();
    createShaderStorageBuffers(); // Compute
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createComputeUniformBuffers(); // Compute
    createGraphicsDescriptorPool();
    createComputeDescriptorPool(); // Compute
    createDescriptorSets();
    createComputeDescriptorSets(); // Compute
    renderWorkerPool.initialize(device, graphicsAndComputeIndex, MAX_FRAMES_IN_FLIGHT); // Multi-threading
    createGraphicsCommandBuffers();
    createComputeCommandBuffers(); // Compute
    createParticleSecondaryCommandBuffers(); // Multithreading
    recordParticleSecondaryCommandBuffer(frameIndex); // Multithreading
    createSyncObjects();
    createComputeSyncObjects(); // Compute
}

void VulkanEngine::mainLoop() {
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)){
            if (event.type == SDL_EVENT_QUIT) {done = true;}
            else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {framebufferResized = true;}
            else if (event.type == SDL_EVENT_WINDOW_MINIMIZED) {minimized = true;}
            else if (event.type == SDL_EVENT_WINDOW_RESTORED) {minimized = false; framebufferResized = true;}
            }
        if (!minimized){
            drawFrame();
        }
    }
    device.waitIdle();
}

void VulkanEngine::cleanup() {
    renderWorkerPool.shutdown();
    for (auto& gameObject : gameObjects) 
    {
        for (size_t i = 0; i < gameObject.uniformBuffersMemory.size(); i++) {
            if (gameObject.uniformBuffersMapped[i] != nullptr) {
                gameObject.uniformBuffersMemory[i].unmapMemory();
            }
        }
        gameObject.uniformBuffers.clear();
        gameObject.uniformBuffersMemory.clear();
        gameObject.uniformBuffersMapped.clear();
        gameObject.descriptorSets.clear();
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void VulkanEngine::createInstance()
{
    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName = "WizApplication",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "WizEngine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14};


    // Get the required layers
    std::vector<char const*> requiredLayers;
    if (enableValidationLayers){
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Check if the required layers are supported by Vulkan implementation.
    auto layerProperties = context.enumerateInstanceLayerProperties();
    auto unsupportedLayerIt = std::ranges::find_if(requiredLayers, [&layerProperties](auto const &requiredLayer) {
        return std::ranges::none_of(layerProperties, [requiredLayer](auto const &layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
        });
    if (unsupportedLayerIt != requiredLayers.end())
    {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }

    // Get the required extensions.
    auto requiredExtensions = getRequiredInstanceExtensions();

    // Check if the required extensions are supported by the Vulkan implementation
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    auto unsupportedPropertyIt = 
        std::ranges::find_if(requiredExtensions,
            [&extensionProperties](auto const &requiredExtension) {
                return std::ranges::none_of(extensionProperties,
                                            [requiredExtension](auto const &extensionProperty) { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; });
                });
    if (unsupportedPropertyIt != requiredExtensions.end())
    {
        throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));                            
    }
        
    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data()};
    instance = vk::raii::Instance(context, createInfo);
};

void VulkanEngine::setupDebugMessenger()
{
    if (!enableValidationLayers) return;
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags,
                                                                        .messageType     = messageTypeFlags,
                                                                        .pfnUserCallback = &debugCallback};
    debugMessenger = instance.createDebugUtilsMessengerEXT( debugUtilsMessengerCreateInfoEXT );
}

std::vector<const char*> VulkanEngine::getRequiredInstanceExtensions(){
    Uint32 sdlExtensionCount = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    std::vector extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);
    if (enableValidationLayers){
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
    return extensions;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanEngine::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                    vk::DebugUtilsMessageTypeFlagsEXT type,
                                                    const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData,
                                                    void * pUserData)
{
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        std::cerr << "validation layer: type " << vk::to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
        Logger(logLevel::Warning, ("validation layer: type " + vk::to_string(type) + " msg: " + pCallbackData->pMessage));
    }

    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        std::cerr << "validation layer: type " << vk::to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
        Logger(logLevel::Error, ("validation layer: type " + vk::to_string(type) + " msg: " + pCallbackData->pMessage));
    }

    return vk::False;
}

void VulkanEngine::createSurface()
{
    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window, *instance, nullptr, &rawSurface))
    {
        throw std::runtime_error(std::string("Failed to create Vulkan surface: ") + SDL_GetError());
    }
    surface = vk::raii::SurfaceKHR(instance, rawSurface);
}

void VulkanEngine::pickPhysicalDevice()
{
    physicalDevice = selectPhysicalDevice(instance);
}

vk::SampleCountFlagBits VulkanEngine::getMaxUsableSampleCount()
{
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

    vk::SampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

    if (counts & vk::SampleCountFlagBits::e64){
        return vk::SampleCountFlagBits::e64;
    }
    if (counts & vk::SampleCountFlagBits::e32){
        return vk::SampleCountFlagBits::e32;
    }
    if (counts & vk::SampleCountFlagBits::e16){
        return vk::SampleCountFlagBits::e16;
    }
    if (counts & vk::SampleCountFlagBits::e8){
        return vk::SampleCountFlagBits::e8;
    }
    if (counts & vk::SampleCountFlagBits::e4){
        return vk::SampleCountFlagBits::e4;
    }
    if (counts & vk::SampleCountFlagBits::e2){
        return vk::SampleCountFlagBits::e2;
    }
    return vk::SampleCountFlagBits::e1;
}

void VulkanEngine::chooseMsaaCount()
{
    vk::SampleCountFlagBits MAX_MSAA = getMaxUsableSampleCount();
    SETTINGS_MSAA < MAX_MSAA ? msaaSamples = SETTINGS_MSAA : msaaSamples = MAX_MSAA;
}

void VulkanEngine::createLogicalDevice()
{
    graphicsAndComputeIndex = findGraphicsPresentationQueueFamily(physicalDevice, surface);
    device = makeLogicalDevice(physicalDevice, graphicsAndComputeIndex);
    graphicsQueue = vk::raii::Queue(device, graphicsAndComputeIndex, 0);
    computeQueue  = vk::raii::Queue(device, graphicsAndComputeIndex, 0);
}

void VulkanEngine::createSwapChain()
{
    std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
    swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

    std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(availablePresentModes);

    vk::SurfaceCapabilitiesKHR  surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    swapChainExtent = chooseSwapExtent(surfaceCapabilities, window);
    uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);
    
    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface = *surface,
        .minImageCount = minImageCount,
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = true,
    };
    swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
    swapChainImages = swapChain.getImages();
}

void VulkanEngine::createImageViews()
{
    swapChainImageViews.clear();
    swapChainImageViews.reserve(swapChainImages.size());

    for (auto &image : swapChainImages)
    {
        swapChainImageViews.emplace_back(createImageView(image, swapChainSurfaceFormat.format, vk::ImageAspectFlagBits::eColor, 1));
    }
}

void VulkanEngine::createGraphicsDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings{
       vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex
        },

        vk::DescriptorSetLayoutBinding{
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
        }
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    graphicsDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void VulkanEngine::createComputeDescriptorSetLayout()
{
        std::array<vk::DescriptorSetLayoutBinding, 3> layoutBindings{
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
        vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
        .pBindings = layoutBindings.data()
    };

    computeDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void VulkanEngine::createGraphicsPipeline()
{
    graphicsPipelineLayout = makePipelineLayout(device, graphicsDescriptorSetLayout);

    vk::Format depthFormat = findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, 
        vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);

    graphicsPipeline = makeGraphicsPipeline(device, graphicsPipelineLayout, swapChainSurfaceFormat.format, depthFormat, msaaSamples);
}

void VulkanEngine::createComputePipeline()
{
    auto computeShaderCode = readfile("shaders/compute.spv");

    vk::raii::ShaderModule shaderModule = createShaderModule(device, computeShaderCode);

    vk::PipelineShaderStageCreateInfo computeShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = *shaderModule,
        .pName = "compMain"
    };

    vk::DescriptorSetLayout computeLayout = *computeDescriptorSetLayout;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 1,
        .pSetLayouts = &computeLayout
    };

    computePipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    vk::ComputePipelineCreateInfo pipelineInfo{
        .stage = computeShaderStageInfo,
        .layout = *computePipelineLayout
    };

    computePipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

void VulkanEngine::createParticlePipeline()
{
    auto particleShaderCode = readfile("shaders/particle.spv");

    vk::raii::ShaderModule particleShaderModule =
        createShaderModule(device, particleShaderCode);
    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *particleShaderModule,
            .pName = "particleVertMain"
        },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *particleShaderModule,
            .pName = "particleFragMain"
        }
    };

    auto bindingDescription = Particle::getBindingDescription();

    auto attributeDescriptions = Particle::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::ePointList,
        .primitiveRestartEnable = vk::False
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};

    particlePipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .scissorCount = 1
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = msaaSamples,
        .sampleShadingEnable = vk::False
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthTestEnable = vk::False,
        .depthWriteEnable = vk::False,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask =
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy, 
        .attachmentCount = 1, 
        .pAttachments = &colorBlendAttachment};

    std::array<vk::DynamicState, 2> dynamicStates{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain{
        vk::GraphicsPipelineCreateInfo{
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = *particlePipelineLayout,
            .renderPass = nullptr
        },

        vk::PipelineRenderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapChainSurfaceFormat.format,
            .depthAttachmentFormat = (findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, 
                                    vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment))
        }
    };

    particlePipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

void VulkanEngine::createCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsAndComputeIndex
    };

    commandPool = vk::raii::CommandPool(device, poolInfo);
}

vk::Format VulkanEngine::findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (vk::Format format : candidates)
    {
        vk::FormatProperties properties = physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features)
        {
            return format;
        }

        if (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }
    
    throw std::runtime_error("Failed to find supported format!");
}

void VulkanEngine::createDepthResources()
{
    depthImageFormat = findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);

    auto [image, imageMemory] = createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthImageFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);

    depthImage = std::move(image);
    depthImageMemory = std::move(imageMemory);

    depthImageView = createImageView(*depthImage, depthImageFormat, vk::ImageAspectFlagBits::eDepth, 1);
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> VulkanEngine::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    vk::BufferCreateInfo bufferInfo{
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    vk::raii::Buffer buffer {device, bufferInfo};

    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
    };

    vk::raii::DeviceMemory bufferMemory{device, allocInfo};

    buffer.bindMemory(*bufferMemory, 0);

    return {std::move(buffer), std::move(bufferMemory)};
}


uint32_t VulkanEngine::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void VulkanEngine::copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size)
{
    vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();

    commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy(0, 0, size));

    endSingleTimeCommands(std::move(commandCopyBuffer));
}

void VulkanEngine::createShaderStorageBuffers()
{
    shaderStorageBuffers.clear();
    shaderStorageBuffersMemory.clear();

    // Initialize particles on CPU
    std::default_random_engine rndEngine(static_cast<unsigned>(time(nullptr)));

    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    std::vector<Particle> particles(PARTICLE_COUNT);

    for (auto& particle : particles)
    {
        float r = 0.25f * sqrtf(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;

        float x = r * cosf(theta) * HEIGHT / WIDTH;
        float y = r * sinf(theta) * HEIGHT / WIDTH;

        particle.position = glm::vec2(x, y);

        particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;

        particle.color = glm::vec4(
            rndDist(rndEngine),
            rndDist(rndEngine),
            rndDist(rndEngine),
            1.0f
        );
    }

    vk::DeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

    auto [stagingBuffer, stagingBufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = stagingBufferMemory.mapMemory(0, bufferSize);

    memcpy(data, particles.data(), static_cast<size_t>(bufferSize));

    stagingBufferMemory.unmapMemory();

    // Make one SSBO per frame in flight
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::raii::Buffer storageBuffer(nullptr);
        vk::raii::DeviceMemory storageBufferMemory(nullptr);

        auto [storageBufferPerFrame, storageBufferMemoryPerFrame] =createBuffer(bufferSize, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        copyBuffer(stagingBuffer, storageBufferPerFrame, bufferSize);

        shaderStorageBuffers.emplace_back(std::move(storageBufferPerFrame));
        shaderStorageBuffersMemory.emplace_back(std::move(storageBufferMemoryPerFrame));
    }
}

void VulkanEngine::createVertexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    auto [stagingBuffer, stagingBufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = stagingBufferMemory.mapMemory(0, bufferSize);

    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));

    stagingBufferMemory.unmapMemory();

    auto [buffer, bufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

    vertexBuffer = std::move(buffer);
    vertexBufferMemory = std::move(bufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}

void VulkanEngine::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    auto [stagingBuffer, stagingBufferMemory] = 
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = stagingBufferMemory.mapMemory(0, bufferSize);

    std::memcpy(data, indices.data(), static_cast<size_t>(bufferSize));

    stagingBufferMemory.unmapMemory();

    auto [buffer, bufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

    indexBuffer = std::move(buffer);
    indexBufferMemory = std::move(bufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void VulkanEngine::createUniformBuffers()
{
    for (auto &gameObject : gameObjects)
    {
        gameObject.uniformBuffers.clear();
        gameObject.uniformBuffersMemory.clear();
        gameObject.uniformBuffersMapped.clear();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

            auto [buffer, bufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

            gameObject.uniformBuffers.emplace_back(std::move(buffer));
            gameObject.uniformBuffersMemory.emplace_back(std::move(bufferMemory));
            gameObject.uniformBuffersMapped.emplace_back(gameObject.uniformBuffersMemory[i].mapMemory(0, bufferSize));
        }
    }
}

void VulkanEngine::createComputeUniformBuffers()
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    computeUniformBuffers.clear();
    computeUniformBuffersMemory.clear();
    computeUniformBuffersMapped.clear();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        auto [buffer, bufferMemory] = createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        computeUniformBuffers.emplace_back(std::move(buffer));
        computeUniformBuffersMemory.emplace_back(std::move(bufferMemory));
        computeUniformBuffersMapped.emplace_back(computeUniformBuffersMemory.back().mapMemory(0, bufferSize));
    }
}

void VulkanEngine::createGraphicsDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes{
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT
        },
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT
        }
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    graphicsDescriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

void VulkanEngine::createComputeDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSizes{
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = MAX_FRAMES_IN_FLIGHT
        },
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = MAX_FRAMES_IN_FLIGHT * 2
        }
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    computeDescriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

void VulkanEngine::createDescriptorSets()
{
    // For each game object
    for (auto& gameObject : gameObjects) 
    {
        // Create descriptor sets for each frame in flight
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *graphicsDescriptorSetLayout);

        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = graphicsDescriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data()
        };
    
        gameObject.descriptorSets.clear();
        gameObject.descriptorSets = device.allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vk::DescriptorBufferInfo bufferInfo{
                .buffer = *gameObject.uniformBuffers[i],
                .offset = 0,
                .range = sizeof(UniformBufferObject)
            };

            vk::DescriptorImageInfo imageInfo{
                .sampler = *textureSampler,
                .imageView = *textureImageView,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            };

            std::array<vk::WriteDescriptorSet, 2> descriptorWrites{
                vk::WriteDescriptorSet{
                    .dstSet = *gameObject.descriptorSets[i],
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                    .pBufferInfo = &bufferInfo
                },
                vk::WriteDescriptorSet{
                    .dstSet = *gameObject.descriptorSets[i],
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                    .pImageInfo = &imageInfo
                }
            };
            device.updateDescriptorSets(descriptorWrites, {});
        }
    }
}

void VulkanEngine::createComputeDescriptorSets()
{
    // Create descriptor sets for each frame in flight
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *computeDescriptorSetLayout);

    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = computeDescriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    computeDescriptorSets.clear();
    computeDescriptorSets = device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = *computeUniformBuffers[i],
            .offset = 0,
            .range = sizeof(ComputeUniformBufferObject)
        };

        size_t previousFrame = (i + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT;

        vk::DescriptorBufferInfo previousStorageBufferInfo{
            .buffer = *shaderStorageBuffers[previousFrame],
            .offset = 0,
            .range = sizeof(Particle) * PARTICLE_COUNT
        };
        vk::DescriptorBufferInfo currentStorageBufferInfo{
            .buffer = *shaderStorageBuffers[i],
            .offset = 0,
            .range = sizeof(Particle) * PARTICLE_COUNT
        };

        std::array<vk::WriteDescriptorSet, 3> descriptorWrites{
            vk::WriteDescriptorSet{
                .dstSet = *computeDescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &bufferInfo
            },
            vk::WriteDescriptorSet{
                .dstSet = *computeDescriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &previousStorageBufferInfo
            },
            vk::WriteDescriptorSet{
                .dstSet = *computeDescriptorSets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &currentStorageBufferInfo
            }
        };
        device.updateDescriptorSets(descriptorWrites, {});
    }
}

void VulkanEngine::updateUniformBuffers(uint32_t currentFrame)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Camera and projection matrices (shared by all objects)
    glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 6.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                     static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height),
                                     0.1f, 20.0f);

    // Update uniform buffers for each object
    for (auto& gameObject : gameObjects) 
    {
        // Apply continuous rotation to the object
        gameObject.rotation.y += 0.001f; // Slow rotation around Y axis

        // Get the model matrix for this object
        glm::mat4 initialRotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 model = gameObject.getModelMatrix() * initialRotation;

        // Create and update the UBO
        UniformBufferObject ubo{
            .model = model,
            .view = view,
            .proj = proj
        };
        // Copy the UBO data to the mapped memory
        std::memcpy(gameObject.uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    }
}

void VulkanEngine::updateComputeUniformBuffer(uint32_t currentFrame, float deltaTime)
{
    ComputeUniformBufferObject ubo{};
    ubo.deltaTime = deltaTime;

    memcpy(computeUniformBuffersMapped[frameIndex], &ubo, sizeof(ComputeUniformBufferObject));
}

void VulkanEngine::createGraphicsCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = *commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    graphicsCommandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void VulkanEngine::recordGraphicsCommandBuffer(uint32_t imageIndex, uint32_t frameIndex)
{
    auto &graphicsCommandBuffer = graphicsCommandBuffers[frameIndex];
    
    graphicsCommandBuffer.begin({});
    
    transitionImageLayout(graphicsCommandBuffer, swapChainImages[imageIndex], vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {},
    vk::AccessFlagBits2::eColorAttachmentWrite, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);

    transitionImageLayout(graphicsCommandBuffer, *colorImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::AccessFlagBits2::eColorAttachmentWrite, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);

    transitionImageLayout(graphicsCommandBuffer, *depthImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal, vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::ImageAspectFlagBits::eDepth);


    vk::ClearValue clearColor = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};

    vk::ClearValue clearDepth = vk::ClearDepthStencilValue{1.0f, 0};

    vk::RenderingAttachmentInfo colorAttachmentInfo{
        .imageView = *colorImageView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,

        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = *swapChainImageViews[imageIndex],
        .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,

        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingAttachmentInfo depthAttachmentInfo{
        .imageView = *depthImageView,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clearDepth
    };

    vk::RenderingInfo renderingInfo{
        .flags = vk::RenderingFlagBits::eContentsSecondaryCommandBuffers,
        .renderArea = {
            .offset = {0, 0},
            .extent = swapChainExtent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        
        .pDepthAttachment = &depthAttachmentInfo
    };

    graphicsCommandBuffer.beginRendering(renderingInfo);

    std::vector<vk::CommandBuffer> secondaryCommandBuffers = renderWorkerPool.getSecondaryCommandBuffers(frameIndex);

    secondaryCommandBuffers.push_back(*particleSecondaryCommandBuffers[frameIndex]);

    graphicsCommandBuffer.executeCommands(secondaryCommandBuffers);

    graphicsCommandBuffer.endRendering();

    transitionImageLayout(graphicsCommandBuffer, swapChainImages[imageIndex], vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits2::eColorAttachmentWrite,
        {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe, vk::ImageAspectFlagBits::eColor);

    graphicsCommandBuffer.end();
}

void VulkanEngine::createComputeCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = *commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    computeCommandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

// MultiThreading
void VulkanEngine::createParticleSecondaryCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = *commandPool,
        .level = vk::CommandBufferLevel::eSecondary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };
    particleSecondaryCommandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void VulkanEngine::recordParticleSecondaryCommandBuffer(uint32_t frameIndex)
{
    auto& commandBuffer = particleSecondaryCommandBuffers[frameIndex];

    commandBuffer.reset();

    // ---------------------------------------------------------
    // Tell Vulkan what dynamic rendering environment this
    // secondary command buffer will execute inside.
    // ---------------------------------------------------------
    vk::CommandBufferInheritanceRenderingInfo renderingInheritance{};

    renderingInheritance.colorAttachmentCount = 1;
    renderingInheritance.pColorAttachmentFormats = &swapChainSurfaceFormat.format;
    renderingInheritance.depthAttachmentFormat = depthImageFormat;
    renderingInheritance.stencilAttachmentFormat = vk::Format::eUndefined;
    renderingInheritance.rasterizationSamples = msaaSamples;

    vk::CommandBufferInheritanceInfo inheritanceInfo{
      .pNext = &renderingInheritance
    };

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        .pInheritanceInfo = &inheritanceInfo
    };
    commandBuffer.begin(beginInfo);

    // ---------------------------------------------------------
    // Particle graphics state
    // ---------------------------------------------------------

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *particlePipeline);

    commandBuffer.bindVertexBuffers(0, {*shaderStorageBuffers[frameIndex]}, {0});

    vk::Viewport viewport{0.0f, static_cast<float>(swapChainExtent.height), static_cast<float>(swapChainExtent.width),
        -static_cast<float>(swapChainExtent.height), 0.0f, 1.0f};

    commandBuffer.setViewport(0,{viewport});

    vk::Rect2D scissor{vk::Offset2D{0, 0}, swapChainExtent};

    commandBuffer.setScissor(0, {scissor});

    commandBuffer.draw(PARTICLE_COUNT, 1, 0, 0);

    commandBuffer.end();
}

void VulkanEngine::recordComputeCommandBuffer(uint32_t frameIndex)
{
    auto &ComputeCommandBuffer = computeCommandBuffers[frameIndex];
    
    ComputeCommandBuffer.begin({});
    
    ComputeCommandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);

    ComputeCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipelineLayout, 0, 
        {*computeDescriptorSets[frameIndex]}, {});

    ComputeCommandBuffer.dispatch(PARTICLE_COUNT / PARTICLES_PER_WORKGROUP, 1, 1);

    ComputeCommandBuffer.end();
}

vk::raii::CommandBuffer VulkanEngine::beginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    vk::raii::CommandBuffer commandBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void VulkanEngine::endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer
    };

    graphicsQueue.submit(submitInfo, nullptr);

    graphicsQueue.waitIdle();
}

void VulkanEngine::transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask,
            vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask, vk::ImageAspectFlags aspectFlags)
{
    vk::ImageMemoryBarrier2 barrier{
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,

        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,

        .oldLayout = oldLayout,
        .newLayout = newLayout,

        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

        .image = image,

        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    
    vk::DependencyInfo dependencyInfo{
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    commandBuffer.pipelineBarrier2(dependencyInfo);
}

void VulkanEngine::transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, vk::raii::Image &image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels)
{
    vk::PipelineStageFlags2 sourceStage;
    vk::AccessFlags2 sourceAccess;

    vk::PipelineStageFlags2 destinationStage;
    vk::AccessFlags2 destinationAccess;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        sourceStage = vk::PipelineStageFlagBits2::eTopOfPipe;

        sourceAccess = {};

        destinationStage = vk::PipelineStageFlagBits2::eTransfer;

        destinationAccess = vk::AccessFlagBits2::eTransferWrite;
    }

    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        sourceStage = vk::PipelineStageFlagBits2::eTransfer;

        sourceAccess = vk::AccessFlagBits2::eTransferWrite;

        destinationStage = vk::PipelineStageFlagBits2::eFragmentShader;

        destinationAccess = vk::AccessFlagBits2::eShaderRead;
    }

    else{
        throw std::runtime_error("Unsupported image layout transition!");
    }

    vk::ImageMemoryBarrier2 barrier{
        .srcStageMask = sourceStage,
        .srcAccessMask = sourceAccess,

        .dstStageMask = destinationStage,
        .dstAccessMask = destinationAccess,

        .oldLayout = oldLayout,
        .newLayout = newLayout,

        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

        .image = *image,

        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vk::DependencyInfo dependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    commandBuffer.pipelineBarrier2(dependencyInfo);
}

void VulkanEngine::copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height)
{
    vk::BufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,

        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },

        .imageOffset= {0, 0, 0},

        .imageExtent = {width, height, 1}
    };

    commandBuffer.copyBufferToImage(*buffer, *image, vk::ImageLayout::eTransferDstOptimal, region);
}

void VulkanEngine::copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height, uint32_t mipLevel, vk::DeviceSize bufferOffset)
{
    vk::BufferImageCopy region{
        .bufferOffset = bufferOffset,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,

        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = mipLevel,
            .baseArrayLayer = 0,
            .layerCount = 1
        },

        .imageOffset= {0, 0, 0},

        .imageExtent = {width, height, 1}
    };

    commandBuffer.copyBufferToImage(*buffer, *image, vk::ImageLayout::eTransferDstOptimal, region);
}

void VulkanEngine::createSyncObjects()
{
    assert(renderFinishedSEMAPHORES.empty() && imageAvailableSEMAPHORES.empty() && graphicsInFlightFences.empty());

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        renderFinishedSEMAPHORES.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo{}));
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        imageAvailableSEMAPHORES.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo{}));

        graphicsInFlightFences.emplace_back(vk::raii::Fence(device, {.flags = vk::FenceCreateFlagBits::eSignaled}));
    }
}

void VulkanEngine::createComputeSyncObjects()
{
    computeFinishedSemaphores.clear();
    computeInFlightFences.clear();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
       computeFinishedSemaphores.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo{}));
       computeInFlightFences.emplace_back(vk::raii::Fence(device, {.flags = vk::FenceCreateFlagBits::eSignaled}));
    }
}

void VulkanEngine::drawFrame()
{
    auto computeFenceResult = device.waitForFences(*computeInFlightFences[frameIndex], vk::True, UINT64_MAX); // For Compute
    if (computeFenceResult != vk::Result::eSuccess){throw std::runtime_error("Failed to wait for compute fence!");}

    auto graphicsFenceResult = device.waitForFences(*graphicsInFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (graphicsFenceResult != vk::Result::eSuccess){throw std::runtime_error("Failed to wait for draw fence!");}

    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *imageAvailableSEMAPHORES[frameIndex], nullptr);
    if (result == vk::Result::eErrorOutOfDateKHR){
        recreateSwapChain();
        return;
    }

    auto &computeCommandBuffer = computeCommandBuffers[frameIndex];
    auto &graphicsCommandBuffer = graphicsCommandBuffers[frameIndex];

    computeCommandBuffer.reset();
    updateComputeUniformBuffer(frameIndex, 1.60f); // For Compute
    recordComputeCommandBuffer(frameIndex);
    device.resetFences(*computeInFlightFences[frameIndex]);
    vk::SubmitInfo computeSubmitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &(*computeCommandBuffer),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &(*computeFinishedSemaphores[frameIndex])
    };
    computeQueue.submit(computeSubmitInfo, *computeInFlightFences[frameIndex]);

    device.resetFences(*graphicsInFlightFences[frameIndex]);

    updateUniformBuffers(frameIndex);

    RenderWorkerPool::RenderContext renderContext{
        .gameObjects = gameObjects,
        .graphicsPipeline = *graphicsPipeline,
        .graphicsPipelineLayout = *graphicsPipelineLayout,
        .vertexBuffer = *vertexBuffer,
        .indexBuffer = *indexBuffer,
        .indexCount = static_cast<uint32_t>(indices.size()),
        .extent = swapChainExtent,
        .colorFormat = swapChainSurfaceFormat.format,
        .depthFormat = depthImageFormat,
        .msaaSamples = msaaSamples
    };

    // ---------------------------------------------------------
    // Tell worker threads to start recording fish.
    // ---------------------------------------------------------
    renderWorkerPool.startWork(frameIndex, renderContext);

    // ---------------------------------------------------------
    // While THEY are working, the MAIN THREAD can also work.
    // ---------------------------------------------------------
    recordParticleSecondaryCommandBuffer(frameIndex);

    // ---------------------------------------------------------
    // Now make sure fish recording has completed.
    // ---------------------------------------------------------
    renderWorkerPool.waitForWorkers();

    // ---------------------------------------------------------
    // All secondary command buffers are ready.
    // Record the primary coordinator.
    // ---------------------------------------------------------
    graphicsCommandBuffer.reset();

    recordGraphicsCommandBuffer(imageIndex, frameIndex);

    std::array<vk::Semaphore, 2> waitSemaphores{
        *imageAvailableSEMAPHORES[frameIndex],
        *computeFinishedSemaphores[frameIndex]
    };

    std::array<vk::PipelineStageFlags, 2> waitDestinationStageMask{
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eVertexInput
    };

    const vk::SubmitInfo graphicsSubmitInfo{
        .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphores = waitSemaphores.data(),
        .pWaitDstStageMask = waitDestinationStageMask.data(),

        .commandBufferCount = 1,
        .pCommandBuffers = &*graphicsCommandBuffer,

        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSEMAPHORES[imageIndex]
    };

    device.resetFences(*graphicsInFlightFences[frameIndex]);
    graphicsQueue.submit(graphicsSubmitInfo, *graphicsInFlightFences[frameIndex]);

    const vk::PresentInfoKHR graphicsPresentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSEMAPHORES[imageIndex],

        .swapchainCount = 1,
        .pSwapchains= &*swapChain,

        .pImageIndices = &imageIndex
    };

    result = graphicsQueue.presentKHR(graphicsPresentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized == true){
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != vk::Result::eSuccess){
        throw std::runtime_error("Failed to present swapchain image!");
    }

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanEngine::cleanupSwapChain()
{
    depthImageView = nullptr;
    depthImage = nullptr;
    depthImageMemory = nullptr;

    colorImageView = nullptr;
    colorImage = nullptr;
    colorImageMemory = nullptr;

    swapChainImageViews.clear();
    swapChainImages.clear();
    swapChain = nullptr;
}

void VulkanEngine::recreateSwapChain()
{
    int width = 0, height = 0;

    if (!SDL_GetWindowSizeInPixels(window, &width, &height)){
        throw std::runtime_error("Failed to get window pixel size: " + std::string(SDL_GetError()));
    }

    if (width == 0 || height == 0) {return;}

    device.waitIdle();

    cleanupSwapChain();
    createSwapChain();
    createImageViews();

    createColorResources();
    createDepthResources();
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory> VulkanEngine::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = numSamples,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    vk::raii::Image image{device, imageInfo};

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
    };

    vk::raii::DeviceMemory imageMemory{device, allocInfo};

    image.bindMemory(*imageMemory, 0);

    return {std::move(image), std::move(imageMemory)};
}

void VulkanEngine::createTextureImage() {
    // It is notable I have not implemented support for KTX2 stored cubemaps and texture arrays

    // Load KTX2 texture instead of using stb_image
    ktxTexture* kTexture;
    KTX_error_code result = ktxTexture_CreateFromNamedFile(TEXTURE_PATH.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kTexture);

    if (result != KTX_SUCCESS)
    {std::cerr << "Failed to load KTX texture\n" << "Path: " << TEXTURE_PATH << '\n' << "KTX error: " << ktxErrorString(result) << '\n';
    throw std::runtime_error("failed to load ktx texture image!");}

    // Get texture dimensions and data
    uint32_t texWidth = kTexture->baseWidth;
    uint32_t texHeight = kTexture->baseHeight;

    // Assuming there are built in mipmaps:
    bool hadMips = kTexture->numLevels > 1;
    if (hadMips){mipLevels = kTexture->numLevels;}
    else{
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    }

    ktx_size_t imageSize = ktxTexture_GetDataSize(kTexture);
    ktx_uint8_t* ktxTextureData = ktxTexture_GetData(kTexture);
    if (imageSize == 0 || ktxTextureData == nullptr){ktxTexture_Destroy(kTexture);
        throw std::runtime_error("KTX texture contains no image data");}

    // Create staging buffer
    auto [stagingBuffer, stagingBufferMemory] = createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    // Copy texture data to staging buffer
    void* data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, ktxTextureData, static_cast<size_t>(imageSize));
    stagingBufferMemory.unmapMemory();

    // Determine the Vulkan format from KTX format
    textureImageFormat = static_cast<vk::Format>(ktxTexture_GetVkFormat(kTexture));

    // Create the texture image
    vk::ImageUsageFlags textureUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    if (!hadMips){textureUsage |= vk::ImageUsageFlagBits::eTransferSrc;}

    auto [image, imageMemory] = createImage(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), mipLevels, vk::SampleCountFlagBits::e1, textureImageFormat, 
    vk::ImageTiling::eOptimal, textureUsage, vk::MemoryPropertyFlagBits::eDeviceLocal);

    textureImage = std::move(image);
    textureImageMemory = std::move(imageMemory);

    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

    // Copy data from staging buffer to texture image
    transitionImageLayout(commandBuffer, textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, mipLevels);

    if (hadMips)
    {
        for (uint32_t i = 0; i < mipLevels; i++)
        {
            ktx_size_t offset = 0;
            KTX_error_code result2 = ktxTexture_GetImageOffset(kTexture, i, 0, 0, &offset);
            if (result2 != KTX_SUCCESS) {ktxTexture_Destroy(kTexture);
                throw std::runtime_error("Failed to get KTX mip offset");}

            uint32_t mipWidth = std::max(1u, texWidth >> i);
            uint32_t mipHeight = std::max(1u, texHeight >> i);

            copyBufferToImage(commandBuffer, stagingBuffer, textureImage, mipWidth, mipHeight, i, static_cast<vk::DeviceSize>(offset));
        }

        transitionImageLayout(commandBuffer, textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, mipLevels);
    }

    if (!hadMips)
    {
    copyBufferToImage(commandBuffer, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    generateMipmaps(commandBuffer, textureImage, textureImageFormat, texWidth, texHeight, mipLevels);
    }

    endSingleTimeCommands(std::move(commandBuffer));

    // Cleanup KTX resources
    ktxTexture_Destroy(kTexture);
}

vk::raii::ImageView VulkanEngine::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    vk::ImageViewCreateInfo viewInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1}
    };

    return vk::raii::ImageView(device, viewInfo);
}

void VulkanEngine::createTextureImageView()
{
    textureImageView = createImageView(*textureImage, textureImageFormat, vk::ImageAspectFlagBits::eColor, mipLevels);
}

void VulkanEngine::createTextureSampler()
{
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,

        .mipmapMode = vk::SamplerMipmapMode::eLinear,

        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,

        .mipLodBias = 0.0f,

        .anisotropyEnable = vk::True,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,

        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,

        .minLod = 0.0f,
        .maxLod = static_cast<float>(mipLevels),

        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = vk::False
    };

    textureSampler = vk::raii::Sampler(device, samplerInfo);
}

int32_t loadGLTFFindAttribute(const tg3_primitive &primitive, std::string_view name)
{
    for (uint32_t i = 0; i < primitive.attributes_count; i++)
    {
        const tg3_str_int_pair &attribute = primitive.attributes[i];
        std::string_view attributeName(attribute.key.data, attribute.key.len);
        if (attributeName == name){
            return attribute.value;
        }
    }
    return TG3_INDEX_NONE;
}

void VulkanEngine::loadGltf(const std::string &path, MeshData &meshData)
{
    tg3_parse_options options{};
    tg3_parse_options_init(&options);

    tg3_error_stack errors{};
    tg3_error_stack_init(&errors);

    tg3_model model{};

    tg3_error_code result = tg3_parse_file(&model, &errors, path.c_str(), static_cast<uint32_t>(path.size()), &options);

    if (result != TG3_OK)
    {
        for (uint32_t i = 0; i < errors.count; i++)
        {
            const tg3_error_entry &error = errors.entries[i];

            std::cerr << "GLTF error: " << (error.message != nullptr ? error.message : "(unknown)") << '\n';
        }
        tg3_model_free(&model);
        tg3_error_stack_free(&errors);

        throw std::runtime_error("Failed to load glTF model");
    }

    for (uint32_t meshIndex = 0; meshIndex < model.meshes_count; meshIndex++)
    {
        const tg3_mesh &mesh = model.meshes[meshIndex];

        for (uint32_t primitiveIndex = 0; primitiveIndex < mesh.primitives_count; primitiveIndex++)
        {
            const tg3_primitive &primitive = mesh.primitives[primitiveIndex];

            // Get indices
            if (primitive.indices == TG3_INDEX_NONE){tg3_model_free(&model); tg3_error_stack_free(&errors);
                throw std::runtime_error("Primitive has no Indices: " + std::string(mesh.name.data, mesh.name.len));}
            const tg3_accessor &indexAccessor = model.accessors[primitive.indices];
            const tg3_buffer_view &tg3_indexBufferView = model.buffer_views[indexAccessor.buffer_view];
            const tg3_buffer &tg3_indexBuffer = model.buffers[tg3_indexBufferView.buffer];
            
            // Get vertex positions
            int32_t positionIndex = loadGLTFFindAttribute(primitive, "POSITION");
            if (positionIndex == TG3_INDEX_NONE){tg3_model_free(&model); tg3_error_stack_free(&errors);
                throw std::runtime_error("Primitive has no POSITION attribute: " + std::string(mesh.name.data, mesh.name.len));}
            const tg3_accessor &posAccessor = model.accessors[positionIndex];
            const tg3_buffer_view &posBufferView = model.buffer_views[posAccessor.buffer_view];
            const tg3_buffer &posBuffer = model.buffers[posBufferView.buffer];

            // Get texture coordinates if available
            bool hasTexCoords = false;
            int32_t texIndex = loadGLTFFindAttribute(primitive, "TEXCOORD_0");
            if (texIndex != TG3_INDEX_NONE){hasTexCoords = true;}
            const tg3_accessor *texCoordAccessor = nullptr;
            const tg3_buffer_view *texCoordBufferView = nullptr;
            const tg3_buffer *texCoordBuffer = nullptr;

            if (hasTexCoords)
            {
                texCoordAccessor = &model.accessors[texIndex];
                texCoordBufferView = &model.buffer_views[texCoordAccessor->buffer_view];
                texCoordBuffer = &model.buffers[texCoordBufferView->buffer];
            }

            uint32_t baseVertex = static_cast<uint32_t>(meshData.vertices.size());

            for (uint32_t i = 0; i < posAccessor.count; i++)
            {
                Vertex vertex{};

                const float *pos = reinterpret_cast<const float *>(&posBuffer.data.data[posBufferView.byte_offset + posAccessor.byte_offset + (i * sizeof(float) * 3)]);
                // glTF uses a right-handed, Y-up coordinate system, while this scene is set up
                // Z-up. Negating the Y coordinate here would mirror the model instead of
                // reorienting it - Vulkan's clip-space Y convention is already handled
                // separately via proj[1][1] *= -1 in updateUniformBuffer, so we keep the
                // position as-is and let a rotation there do the Y-up to Z-up conversion.
                vertex.pos = {pos[0], pos[1], pos[2]};

                if (hasTexCoords){
                    const float *texCoord = reinterpret_cast<const float *>(&texCoordBuffer->data.data[texCoordBufferView->byte_offset + texCoordAccessor->byte_offset + (i * sizeof(float) * 2)]);
                    vertex.texCoord = {texCoord[0], texCoord[1]};
                }
                else{
                    vertex.texCoord = {0.0f, 0.0f};
                }
                vertex.color = {1.0f, 1.0f, 1.0f};

                meshData.vertices.push_back(vertex);
            }

            const unsigned char *indexData   = &tg3_indexBuffer.data.data[tg3_indexBufferView.byte_offset + indexAccessor.byte_offset];
            size_t               indexCount  = indexAccessor.count;
            size_t               indexStride = 0;

            // Determine index stride based on component type
            if (indexAccessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                indexStride = sizeof(uint16_t);
            }
            else if (indexAccessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_INT)
            {
                indexStride = sizeof(uint32_t);
            }
            else if (indexAccessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                indexStride = sizeof(uint8_t);
            }
            else
            {
                tg3_model_free(&model); 
                tg3_error_stack_free(&errors);
                throw std::runtime_error("Unsupported index component type");
            }

            meshData.indices.reserve(meshData.indices.size() + indexCount);

             for (size_t i = 0; i < indexCount; i++)
            {
                uint32_t index = 0;

                if (indexAccessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    index = *reinterpret_cast<const uint16_t *>(indexData + i * indexStride);
                }
                else if (indexAccessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    index = *reinterpret_cast<const uint32_t *>(indexData + i * indexStride);
                }
                else if (indexAccessor.component_type == TG3_COMPONENT_TYPE_UNSIGNED_BYTE)
                {
                    index = *reinterpret_cast<const uint8_t *>(indexData + i * indexStride);
                }

                meshData.indices.push_back(baseVertex + index);
            }
        }
    }
    std::cout
        << "glTF loaded successfully\n"
        << "Buffers: "
        << model.buffers_count << '\n'
        << "Buffer views: "
        << model.buffer_views_count << '\n'
        << "Accessors: "
        << model.accessors_count << '\n'
        << "Meshes: "
        << model.meshes_count << '\n';

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

void VulkanEngine::loadObj(const std::string &path, MeshData &meshData)
{

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())){
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                // 1.0f -  flips the texture coords. tinyobj loader and the vulkan loader have the friction
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = {1.0f, 1.0f, 1.0f};

            auto [it, inserted] = uniqueVertices.insert({vertex, static_cast<uint32_t>(meshData.vertices.size())});
            if (inserted){
                meshData.vertices.push_back(vertex);
            }

            meshData.indices.push_back(it->second);
        }
    }
}

enum class FileType
{
    GLTF,
    GLB,    
    OBJ,
    Unsupported
};

FileType findFileType(const std::string &file_path)
{
    std::string extension = std::filesystem::path(file_path).extension().string();

    if (extension == ".gltf")
    {
        return FileType::GLTF;
    }
    if (extension == ".glb")
    {
        return FileType::GLB;
    }
    if (extension == ".obj")
    {
        return FileType::OBJ;
    }
    return FileType::Unsupported;
}

void VulkanEngine::loadModel()
{
    FileType fileType = findFileType(MODEL_PATH);
    MeshData meshData;
    switch (fileType)
    {
    case FileType::GLTF:
    case FileType::GLB:
        loadGltf(MODEL_PATH, meshData);
        break;
    case FileType::OBJ:
        loadObj(MODEL_PATH, meshData);
        break;
    case FileType::Unsupported:
    default:
        std::cerr << std::string("Unsupported File Type loadModel()") << MODEL_PATH << '\n';
        Logger(logLevel::Warning, std::string("Unsupported File Type") + MODEL_PATH);
        return;
    }
    vertices = std::move(meshData.vertices);
    indices = std::move(meshData.indices);
}

void VulkanEngine::generateMipmaps(vk::raii::CommandBuffer &commandBuffer, vk::raii::Image &image, vk::Format imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels)
{
    vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)){
        throw std::runtime_error("Texture image format does not support linear blitting!");
    }

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        vk::ImageMemoryBarrier2 toTransferSrc{
            .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,

            .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .dstAccessMask = vk::AccessFlagBits2::eTransferRead,

            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eTransferSrcOptimal,

            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

            .image = *image,

            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = i - 1,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
    
        vk::DependencyInfo toTransferSrcDependency{
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &toTransferSrc
        };

        commandBuffer.pipelineBarrier2(toTransferSrcDependency);

        vk::ImageBlit blit{};

        blit.srcSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = i - 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };

        blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};

        blit.dstSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = i,
            .baseArrayLayer = 0,
            .layerCount = 1
        };

        blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};

        commandBuffer.blitImage(*image, vk::ImageLayout::eTransferSrcOptimal, *image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);
    
        vk::ImageMemoryBarrier2 toShaderRead{
            .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .srcAccessMask = vk::AccessFlagBits2::eTransferRead,

            .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
            .dstAccessMask = vk::AccessFlagBits2::eShaderRead,

            .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,

            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

            .image = *image,

            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = i - 1,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
    
        vk::DependencyInfo toShaderReadDependency{
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &toShaderRead
        };

        commandBuffer.pipelineBarrier2(toShaderReadDependency);

        if (mipWidth > 1){mipWidth /= 2;}
        if (mipHeight > 1){mipHeight /= 2;}
    }

    vk::ImageMemoryBarrier2 lastMipBarrier{
            .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,

            .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
            .dstAccessMask = vk::AccessFlagBits2::eShaderRead,

            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,

            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

            .image = *image,

            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = mipLevels - 1,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
    
        vk::DependencyInfo lastMipDependency{
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &lastMipBarrier
        };

        commandBuffer.pipelineBarrier2(lastMipDependency);
}

void VulkanEngine::createColorResources()
{
    vk::Format colorFormat = swapChainSurfaceFormat.format;

    auto [image, imageMemory] = createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, vk::ImageTiling::eOptimal, 
    vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);

    colorImage = std::move(image);
    colorImageMemory = std::move(imageMemory);

    colorImageView = createImageView(*colorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
}

// Initialize the game objects with different positions, rotations, and scales
void VulkanEngine::setupGameObjects() {
    // Object 1 - Center
    gameObjects[0].position = {0.0f, 0.0f, 0.0f};
    gameObjects[0].rotation = {0.0f, 0.0f, 0.0f};
    gameObjects[0].scale = {5.0f, 5.0f, 5.0f};

    // Object 2 - Left
    gameObjects[1].position = {-2.0f, 0.0f, -1.0f};
    gameObjects[1].rotation = {0.0f, glm::radians(45.0f), 0.0f};
    gameObjects[1].scale = {3.0f, 3.0f, 3.0f};

    // Object 3 - Right
    gameObjects[2].position = {2.0f, 0.0f, -1.0f};
    gameObjects[2].rotation = {0.0f, glm::radians(-45.0f), 0.0f};
    gameObjects[2].scale = {0.75f, 0.75f, 0.75f};
}

// OLD vk_pipeline.cpp
std::vector<char> VulkanEngine::readfile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()){
        throw std::runtime_error("Couldn't open shader file" + filename);
    }

    std::vector<char> buffer(static_cast<size_t>(file.tellg()));

    file.seekg(0, std::ios::beg);

    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();

    return buffer;
}

vk::raii::ShaderModule VulkanEngine::createShaderModule(const vk::raii::Device &device, const std::vector<char> &code)
{
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    return vk::raii::ShaderModule(device, createInfo);
}

vk::raii::PipelineLayout VulkanEngine::makePipelineLayout(const vk::raii::Device &device, const vk::raii::DescriptorSetLayout &descriptorSetLayout)
{
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
    .setLayoutCount = 1,
    .pSetLayouts = &*descriptorSetLayout,
    .pushConstantRangeCount = 0
    };

    return vk::raii::PipelineLayout(device, pipelineLayoutInfo);
}

vk::raii::Pipeline VulkanEngine::makeGraphicsPipeline(const vk::raii::Device &device, const vk::raii::PipelineLayout &graphicsPipelineLayout, const vk::Format &colorFormat, const vk::Format &depthFormat, const vk::SampleCountFlagBits &msaaSamples)
{
    auto shaderCode = readfile("shaders/graphics.spv");

    vk::raii::ShaderModule shaderModule = createShaderModule(device, shaderCode);

    vk::PipelineShaderStageCreateInfo vertexShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"
    };

    vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain"
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertexShaderStageInfo,
        fragmentShaderStageInfo
    };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,

        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
    
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false
    };

    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .scissorCount = 1
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = msaaSamples,
        .sampleShadingEnable = vk::False
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,

        .depthBoundsTestEnable = vk::False,

        .stencilTestEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask = 
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA 
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };


    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
    {   .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = *graphicsPipelineLayout,
        .renderPass = nullptr
    },
    {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
        .depthAttachmentFormat = depthFormat
    }
    };

    return vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}
