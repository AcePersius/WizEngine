#include <iostream>
#include <cstdlib>

#include <logs/logger.hpp>
#include <vulkan/vk_engine.hpp>

int main(int argc, char* argv[]) 
{
    initLogger("include/logs/latest.log");

    try
    {
        VulkanEngine engine;
        engine.run();
    }
    catch (const std::exception& e)
    {
        Logger(logLevel::Error, "Error: " + std::string(e.what()));
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    shutdownLogger();
    return EXIT_SUCCESS;
}
