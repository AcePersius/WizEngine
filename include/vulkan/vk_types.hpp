#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// Define a structure to hold per-object data
struct GameObject {
    // Transform properties
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};

    // Uniform buffer for this object (one per frame in flight)
    std::vector<vk::raii::Buffer> uniformBuffers;
    std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    // Descriptor sets for this object (one per frame in flight)
    std::vector<vk::raii::DescriptorSet> descriptorSets;

    // Calculate model matrix based on position, rotation, and scale
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }
};


struct Particle
{
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        return {
            .binding = 0,
            .stride = sizeof(Particle),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static std::array<vk::VertexInputAttributeDescription, 2>
    getAttributeDescriptions()
    {
        return {
            vk::VertexInputAttributeDescription{
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = offsetof(Particle, position)
            },

            vk::VertexInputAttributeDescription{
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32A32Sfloat,
                .offset = offsetof(Particle, color)
            }
        };
    }
};

struct ComputeUniformBufferObject
{
    float deltaTime = 1.0f;
};

struct Vertex{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        return{
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }
    static std::array<vk::VertexInputAttributeDescription, 3>getAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 3> attributes;

        attributes[0] = vk::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, pos)
        };

        attributes[1] = vk::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, color)
        };

        attributes[2] = vk::VertexInputAttributeDescription{
            .location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(Vertex, texCoord)
        };
        return attributes;
    }

    bool operator==(const Vertex &other) const
    {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std
{
    template<>
    struct hash<Vertex>
    {
        size_t operator()(Vertex const &vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};