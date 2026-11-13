#include <logs/logger.hpp>

#include <fstream>
#include <iostream>
#include <string>


std::ofstream logFile;

void initLogger(std::string const filepath)
{
    logFile.open(filepath, std::ios::out | std::ios::trunc);

    if (!logFile.is_open())
    {
        std::cerr << "Failed to initialize Logger";
    }
    int version = SDL_GetVersion();
    std::string SDL_Version = "SDL version: " + std::to_string(SDL_VERSIONNUM_MAJOR(version)) + "." + 
    std::to_string(SDL_VERSIONNUM_MINOR(version)) + "." + std::to_string(SDL_VERSIONNUM_MICRO(version));
    
    Logger(logLevel::Info, SDL_Version);

    uint32_t vulkanVersion = VK_API_VERSION_1_0;
    VkResult result = vkEnumerateInstanceVersion(&vulkanVersion);

    if (result == VK_SUCCESS)
    {Logger(logLevel::Info, "Vulkan API Version: " + std::to_string(VK_API_VERSION_MAJOR(vulkanVersion)) + "." +
    std::to_string(VK_API_VERSION_MINOR(vulkanVersion)) + "." + std::to_string(VK_API_VERSION_PATCH(vulkanVersion)) + '\n');}
    else
    {Logger(logLevel::Error, "Failed to query Vulkan version.\n");}
}

std::string logLevelString(logLevel level)
{
    switch (level)
    {
    case logLevel::Info:
    return "[Info] ";

    case logLevel::Warning:
    return "[Warning] ";

    case logLevel::Error:
    return "[Error] ";

    default:
    return "[Unknown]";
    }
}

void Logger(logLevel level, std::string const &message)
{
    if (!logFile.is_open())
    {
        return;
    }
    std::string levelstr = logLevelString(level);
    logFile << levelstr
    << message << "\n";
    logFile.flush();
}

void shutdownLogger()
{
    if (logFile.is_open())
    {
        logFile.close();
    }
}
