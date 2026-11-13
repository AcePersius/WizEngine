#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <string>

enum struct logLevel{
    Info,
    Warning,
    Error
};

void initLogger(std::string const filepath);

void Logger(logLevel level, std::string const &message);

void shutdownLogger();

#endif