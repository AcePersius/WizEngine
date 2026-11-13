#pragma once

#include <vulkan/vk_config.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <SDL3/SDL.h>

vk::raii::PhysicalDevice selectPhysicalDevice(const vk::raii::Instance &instance);
uint32_t findGraphicsPresentationQueueFamily(const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::SurfaceKHR &surface);
vk::raii::Device makeLogicalDevice(const vk::raii::PhysicalDevice &physicalDevice, std::uint32_t const &graphicsIndex);

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities, SDL_Window *window);
std::uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities);