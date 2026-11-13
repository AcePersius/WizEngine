#include <vulkan/vk_config.hpp>
#include <vulkan/vk_device.hpp>
#include <SDL3/SDL.h>

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <vector>
#include <map>

std::vector<const char*> requiredDeviceExtension = {vk::KHRSwapchainExtensionName};

bool isDeviceSuitable(const vk::raii::PhysicalDevice &physicalDevice)
{
    bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

    auto queueFamilies = physicalDevice.getQueueFamilyProperties();

    bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const &qfp)
    {return !!(qfp.queueFlags &vk::QueueFlagBits::eGraphics);});

    auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();

    bool supportsAllRequiredExtensions = 
    std::ranges::all_of(requiredDeviceExtension, [&availableDeviceExtensions](auto const &requiredExtension)
    {
        return std::ranges::any_of(availableDeviceExtensions, [requiredExtension](auto const &availableExtension)
        {
            return strcmp(availableExtension.extensionName, requiredExtension) == 0;
        });
    });

    auto features = 
    physicalDevice.template getFeatures2<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features,
    vk::PhysicalDeviceVulkan13Features,
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

    bool supportsRequiredFeatures =
    features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
    features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
    features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
    features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
    features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;


    return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
}

uint32_t scoreDevice(const vk::raii::PhysicalDevice &physicalDevice)
{
    auto properties = physicalDevice.getProperties();

    uint32_t score = 0;

    if(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
    {score += 1000;}

    score += properties.limits.maxImageDimension2D;

    return score;
}

vk::raii::PhysicalDevice selectPhysicalDevice(const vk::raii::Instance &instance)
{
    auto physicalDevices = instance.enumeratePhysicalDevices();

    if (physicalDevices.empty())
    {
        throw std::runtime_error("Failed to find GPU's with Vulkan Support!");
    }

    std::multimap<int, vk::raii::PhysicalDevice> candidates;
    for (const auto &physicalDevice : physicalDevices)
    {
        if (isDeviceSuitable(physicalDevice))
        {
            auto score = scoreDevice(physicalDevice);
            candidates.insert(std::make_pair(score, physicalDevice));
        }
    }

    if (candidates.empty())
    {
        throw std::runtime_error("Failed to find a suitable GPU.");
    }

    return candidates.rbegin()->second;
}

uint32_t findGraphicsPresentationQueueFamily(const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::SurfaceKHR &surface)
{
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    
    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
    {
        bool supportsGraphicsAndCompute = !!((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) 
        && (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute));
        bool supportsPresentation = physicalDevice.getSurfaceSupportKHR(i, *surface);

        if (supportsGraphicsAndCompute && supportsPresentation){
            return i;
        }
    }

    throw std::runtime_error("Could not find a queue family supporting graphics, compute, and presentation.");
}

vk::raii::Device makeLogicalDevice(const vk::raii::PhysicalDevice &physicalDevice, std::uint32_t const &graphicsIndex)
{
    float queuePriority = 0.5f;

    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = graphicsIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    vk::StructureChain<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features,
    vk::PhysicalDeviceVulkan13Features,
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {.features = {.samplerAnisotropy = true}},
        {.shaderDrawParameters = true},
        {.synchronization2 = true, .dynamicRendering = true},
        {.extendedDynamicState = true}
     };

     vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
        .ppEnabledExtensionNames = requiredDeviceExtension.data()
     };

    return vk::raii::Device(physicalDevice, deviceCreateInfo);
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
    {
        if (availableFormats.empty()){
            throw std::runtime_error("No swapchain surface formats available.");
        }

        auto formatIt = std::ranges::find_if(availableFormats, [](const auto &format)
        {return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;});
        if (formatIt != availableFormats.end()){
            return *formatIt;
        }

        return availableFormats[0];
    }

vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
{
    auto presentModeIt = std::ranges::find(availablePresentModes, vk::PresentModeKHR::eMailbox);
    if (presentModeIt != availablePresentModes.end()){
        return vk::PresentModeKHR::eMailbox;
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities, SDL_Window *window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()){
        return capabilities.currentExtent;
    }
    int width, height {0};

    if (!SDL_GetWindowSizeInPixels(window, &width, &height)){
        throw std::runtime_error(("Failed to get window pixel size: ") + std::string(SDL_GetError()));
    }

    std::uint32_t clampedWidth = std::clamp<uint32_t>(static_cast<uint32_t>(width), 
    capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    std::uint32_t clampedHeight = std::clamp<uint32_t>(static_cast<uint32_t>(height),
    capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return {clampedWidth, clampedHeight};
}

std::uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities)
{
    uint32_t minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount){
        minImageCount = surfaceCapabilities.maxImageCount;
    }

    return minImageCount;
}