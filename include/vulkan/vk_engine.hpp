#pragma once

#include <vulkan/vk_config.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vk_types.hpp>
#include <SDL3/SDL.h>

#include <iostream>
#include <span>
#include <vector>

// Multi-Threading
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>



// NUMBER OF FRAMES WITH RESOURCES TIED TO THEM AT ONE TIME
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
// Define the number of objects to render
constexpr int MAX_OBJECTS = 3;
// Particle Count
constexpr uint32_t PARTICLE_COUNT = 8192;
// Number of Compute threads/ invocations per Workgroup
constexpr uint32_t PARTICLES_PER_WORKGROUP = 256;
// Speed of Sim steps
constexpr float PARTICLE_SIMULATION_SPEED = 1.60f;
// Number of worker threads
constexpr uint32_t RENDER_WORKER_COUNT = 2;
// User Defined MSAA Max
constexpr vk::SampleCountFlagBits SETTINGS_MSAA = vk::SampleCountFlagBits::e4;


class RenderWorkerPool
{
public:

    struct RenderContext
        {
            std::span<const GameObject> gameObjects;

            vk::Pipeline graphicsPipeline;
            vk::PipelineLayout graphicsPipelineLayout;

            vk::Buffer vertexBuffer;
            vk::Buffer indexBuffer;

            uint32_t indexCount;

            vk::Extent2D extent;

            vk::Format colorFormat;
            vk::Format depthFormat;

            vk::SampleCountFlagBits msaaSamples;
        };

        std::vector<vk::CommandBuffer> getSecondaryCommandBuffers(uint32_t frameIndex) const
        {
            std::vector<vk::CommandBuffer> commandBuffers;
            commandBuffers.reserve(workers.size());

            for (const auto& worker : workers)
            {
                commandBuffers.push_back(*worker.secondaryCommandBuffers[frameIndex]);
            }

            return commandBuffers;
        }

    void initialize(const vk::raii::Device& device, uint32_t queueFamilyIndex, uint32_t framesInFlight)
    {
        workers.clear();
        workers.resize(RENDER_WORKER_COUNT);

        shouldExit = false;

        // Create one Vulkan command pool per worker
        for (auto& worker : workers)
        {
            vk::CommandPoolCreateInfo poolInfo{
                .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                .queueFamilyIndex = queueFamilyIndex
            };
            worker.commandPool = vk::raii::CommandPool(device, poolInfo);

            vk::CommandBufferAllocateInfo allocInfo{
                .commandPool = *worker.commandPool,
                .level = vk::CommandBufferLevel::eSecondary,
                .commandBufferCount = framesInFlight
            };
            worker.secondaryCommandBuffers = vk::raii::CommandBuffers(device, allocInfo);
        }


        // Launch persistent worker threads
        for (uint32_t i = 0; i < workers.size(); ++i)
        {
            workers[i].thread = std::thread(&RenderWorkerPool::workerThreadMain, this, i);
        }
    }


    void startWork(uint32_t frameIndex, const RenderContext& context)
    {
        {
            std::lock_guard<std::mutex> lock(workMutex);

            uint32_t workerCount = static_cast<uint32_t>(workers.size());
            uint32_t totalObjectCount = static_cast<uint32_t>(context.gameObjects.size());
            uint32_t objectsPerWorker = totalObjectCount / workerCount;
            uint32_t remainder = totalObjectCount % workerCount;
            uint32_t firstObject = 0;

            for (uint32_t i = 0; i < workerCount; i++)
            {
                uint32_t objectCount = objectsPerWorker + (i < remainder ? 1 : 0);

                workers[i].job.frameIndex = frameIndex;
                workers[i].job.firstObject = firstObject;
                workers[i].job.objectCount = objectCount;
                workers[i].job.context = context;

                workers[i].workFinished = false;
                workers[i].workReady = true;

                firstObject += objectCount;
            }
        }
        workAvailableCV.notify_all();
    }


    void waitForWorkers()
    {
        std::unique_lock<std::mutex> lock(workMutex);

        workFinishedCV.wait(lock, [this]
            {
                for (const auto& worker : workers)
                {
                    if (!worker.workFinished)
                    {
                        return false;
                    }
                }
                return true;
            }
        );
    }


    void shutdown()
    {
        shouldExit = true;

        // Wake sleeping workers so they can see shouldExit
        workAvailableCV.notify_all();

        for (auto& worker : workers)
        {
            if (worker.thread.joinable())
            {
                worker.thread.join();
            }
        }
        workers.clear();
    }


private:

    struct RenderJob
    {
        uint32_t frameIndex = 0;

        uint32_t firstObject = 0;
        uint32_t objectCount = 0;

        RenderContext context;
    };

    struct Worker
        {
            vk::raii::CommandPool commandPool = nullptr;
            std::vector<vk::raii::CommandBuffer> secondaryCommandBuffers;
            std::thread thread;

            bool workReady = false;
            bool workFinished = true;

            RenderJob job;
        };

    std::vector<Worker> workers;
    std::atomic<bool> shouldExit{false};
    std::mutex workMutex;
    std::condition_variable workAvailableCV;
    std::condition_variable workFinishedCV;


    void recordSecondaryCommandBuffer(Worker& worker, const RenderJob& job)
    {
        auto& commandBuffer = worker.secondaryCommandBuffers[job.frameIndex];

        commandBuffer.reset();

        // ---------------------------------------------------------
        // Dynamic rendering inheritance
        // ---------------------------------------------------------

        vk::CommandBufferInheritanceRenderingInfo renderingInheritance{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &job.context.colorFormat,
            .depthAttachmentFormat = job.context.depthFormat,
            .stencilAttachmentFormat = vk::Format::eUndefined,
            .rasterizationSamples = job.context.msaaSamples
        };

        vk::CommandBufferInheritanceInfo inheritanceInfo{
            .pNext = &renderingInheritance
        };

        vk::CommandBufferBeginInfo beginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
            .pInheritanceInfo = &inheritanceInfo
        };


        commandBuffer.begin(beginInfo);


        // ---------------------------------------------------------
        // Graphics state
        // ---------------------------------------------------------

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, job.context.graphicsPipeline);

        commandBuffer.bindVertexBuffers(0, {job.context.vertexBuffer}, {0});

        commandBuffer.bindIndexBuffer(
            job.context.indexBuffer,
            0,
            vk::IndexType::eUint32
        );

        commandBuffer.setViewport(0,
            vk::Viewport(0.0f, static_cast<float>(job.context.extent.height), static_cast<float>(job.context.extent.width),
                -static_cast<float>(job.context.extent.height), 0.0f, 1.0f)
        );

        commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), job.context.extent));

        // ---------------------------------------------------------
        // Record this worker's assigned objects
        // ---------------------------------------------------------

        uint32_t endObject = job.firstObject + job.objectCount;

        for (uint32_t i = job.firstObject; i < endObject; ++i)
        {
            const GameObject& gameObject = job.context.gameObjects[i];

            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                job.context.graphicsPipelineLayout,
                0,
                {*gameObject.descriptorSets[job.frameIndex]},
                {}
            );

            commandBuffer.drawIndexed(
                job.context.indexCount,
                1,
                0,
                0,
                0
            );
        }

        commandBuffer.end();
    }

    void workerThreadMain(uint32_t workerIndex)
    {
        Worker& worker = workers[workerIndex];

        while (true)
        {
            std::unique_lock<std::mutex> lock(workMutex);

            // Sleep until this worker has work,
            // or the engine is shutting down
            workAvailableCV.wait(lock, [this, &worker]
                {
                    return shouldExit || worker.workReady;
                }
            );

            if (shouldExit){
                return;
            }

            // This worker has accepted its current job
            worker.workReady = false;

            // Copy the job while we're protected by the mutex.
            RenderJob job = worker.job;

            // Release the mutex while doing the actual work
            lock.unlock();

            // Real Work
            recordSecondaryCommandBuffer(worker, job);

            // Mark the work as finished
            lock.lock();
            worker.workFinished = true;
            workFinishedCV.notify_one();
        }
    }
};

class VulkanEngine{
    public:
        void run();
    
    private:

        RenderWorkerPool renderWorkerPool;

        SDL_Window* window = nullptr;
        bool done = false;
        bool framebufferResized = false;
        bool minimized = false;

        //TEMPY
        // const std::string MODEL_PATH = "models/viking_room.obj";
        // const std::string MODEL_PATH = "models/DamagedHelmet.glb";
        const std::string MODEL_PATH = "models/BarramundiFish.glb";

        //const std::string TEXTURE_PATH = "textures/viking_room.png";
        const std::string TEXTURE_PATH = "textures/white_square.ktx2";

        vk::raii::Context context;
        vk::raii::Instance instance = nullptr;
        vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
        vk::raii::SurfaceKHR surface = nullptr;
        vk::raii::PhysicalDevice physicalDevice = nullptr;
        vk::raii::Device device = nullptr;
        vk::raii::Queue graphicsQueue = nullptr;
        uint32_t graphicsAndComputeIndex {};
        vk::raii::SwapchainKHR swapChain = nullptr;
        std::vector<vk::Image> swapChainImages;
        vk::SurfaceFormatKHR swapChainSurfaceFormat;
        vk::Extent2D swapChainExtent;
        std::vector<vk::raii::ImageView> swapChainImageViews;
        std::array<GameObject, MAX_OBJECTS> gameObjects; // Array of game objects to render
        vk::raii::DescriptorSetLayout graphicsDescriptorSetLayout = nullptr;
        vk::raii::PipelineLayout graphicsPipelineLayout = nullptr;
        vk::raii::Pipeline graphicsPipeline = nullptr;
        vk::raii::CommandPool commandPool = nullptr;
        std::vector<vk::raii::CommandBuffer> graphicsCommandBuffers;
        std::vector<vk::raii::Semaphore> renderFinishedSEMAPHORES;
        std::vector<vk::raii::Semaphore> imageAvailableSEMAPHORES;
        std::vector<vk::raii::Fence> graphicsInFlightFences;
        uint32_t frameIndex = 0;
        vk::raii::DeviceMemory vertexBufferMemory = nullptr;
        vk::raii::Buffer vertexBuffer = nullptr;
        vk::raii::DeviceMemory indexBufferMemory = nullptr;
        vk::raii::Buffer indexBuffer = nullptr;
        vk::raii::DescriptorPool graphicsDescriptorPool = nullptr;
        vk::raii::Queue computeQueue = nullptr; // For Compute
        vk::raii::DescriptorPool computeDescriptorPool = nullptr; // For Compute
        std::vector<vk::raii::DeviceMemory> shaderStorageBuffersMemory; // For Compute
        std::vector<vk::raii::Buffer> shaderStorageBuffers; // For Compute
        std::vector<vk::raii::DeviceMemory> computeUniformBuffersMemory; // For Compute
        std::vector<vk::raii::Buffer> computeUniformBuffers; // For Compute
        std::vector<void*> computeUniformBuffersMapped; // For Compute
        vk::raii::DescriptorSetLayout computeDescriptorSetLayout = nullptr; // For Compute
        std::vector<vk::raii::DescriptorSet> computeDescriptorSets; // For Compute
        vk::raii::PipelineLayout computePipelineLayout = nullptr; // For Compute
        vk::raii::Pipeline computePipeline = nullptr; // For Compute
        std::vector<vk::raii::CommandBuffer> computeCommandBuffers; // For Compute

        std::vector<vk::raii::CommandBuffer> particleSecondaryCommandBuffers; // For Compute Multithread

        std::vector<vk::raii::Semaphore> computeFinishedSemaphores; // For Compute
        std::vector<vk::raii::Fence> computeInFlightFences; // For Compute
        vk::raii::PipelineLayout particlePipelineLayout = nullptr;
        vk::raii::Pipeline particlePipeline = nullptr;

        vk::raii::DeviceMemory textureImageMemory = nullptr;
        vk::raii::Image textureImage = nullptr;
        vk::Format textureImageFormat;
        vk::raii::ImageView textureImageView = nullptr;
        vk::raii::Sampler textureSampler = nullptr;
        vk::Format depthImageFormat;
        vk::raii::DeviceMemory depthImageMemory = nullptr;
        vk::raii::Image depthImage = nullptr;
        vk::raii::ImageView depthImageView = nullptr;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        uint32_t mipLevels = 1;
        vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
        vk::raii::DeviceMemory colorImageMemory = nullptr;
        vk::raii::Image colorImage = nullptr;
        vk::raii::ImageView colorImageView = nullptr;


        void initWindow();
        void initVulkan();
        void mainLoop();
        void cleanup();
        void createInstance();
        void setupDebugMessenger();
        void createSurface();
        void pickPhysicalDevice();
        vk::SampleCountFlagBits getMaxUsableSampleCount();
        void chooseMsaaCount();
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createGraphicsDescriptorSetLayout();
        void createComputeDescriptorSetLayout(); // For Compute
        void createGraphicsPipeline();
        void createComputePipeline();
        void createParticlePipeline();
        void createCommandPool();
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
        std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        void copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size);
        // For Compute
        void createShaderStorageBuffers();
        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffers();
        void createComputeUniformBuffers();
        void createGraphicsDescriptorPool();
        void createComputeDescriptorPool();
        void createDescriptorSets();
        void createComputeDescriptorSets();
        void updateUniformBuffers(uint32_t currentFrame);
        void updateComputeUniformBuffer(uint32_t currentFrame, float deltaTime); // For Compute
        void createGraphicsCommandBuffers();
        void recordGraphicsCommandBuffer(uint32_t imageIndex, uint32_t frameIndex);
        void createComputeCommandBuffers(); // For Compute

        void createParticleSecondaryCommandBuffers(); // For compute Multi-threading
        void recordParticleSecondaryCommandBuffer(uint32_t frameIndex); // For compute Multi-threading

        void recordComputeCommandBuffer(uint32_t frameIndex); // For Compute
        vk::raii::CommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer);
        void transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
            vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask, vk::ImageAspectFlags aspectFlags);
        void transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, vk::raii::Image &image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);
        void copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height);
        void copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height,  uint32_t mipLevel, vk::DeviceSize bufferOffset);
        void createSyncObjects();
        void createComputeSyncObjects(); // For Compute
        void drawFrame();
        void cleanupSwapChain();
        void recreateSwapChain();
        std::pair<vk::raii::Image, vk::raii::DeviceMemory>createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
        void createTextureImage();
        void createTextureImageView();
        void createTextureSampler();
        void loadGltf(const std::string &path, MeshData &meshData);
        void loadObj(const std::string &path, MeshData &meshData);
        void loadModel();
        void setupGameObjects();
        vk::raii::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
        void createDepthResources();
        vk::Format findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
        void generateMipmaps(vk::raii::CommandBuffer &commandBuffer, vk::raii::Image &image, vk::Format imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);
        void createColorResources();
        std::vector<const char*> getRequiredInstanceExtensions();
        static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
            vk::DebugUtilsMessageTypeFlagsEXT   type,
            const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData,
            void * pUserData);

        // Old vk_pipeline.hpp
        std::vector<char> readfile(const std::string &filename);
        vk::raii::ShaderModule createShaderModule(const vk::raii::Device &device, const std::vector<char> &code);
        vk::raii::PipelineLayout makePipelineLayout(const vk::raii::Device &device, const vk::raii::DescriptorSetLayout &descriptorSetLayout);
        vk::raii::Pipeline makeGraphicsPipeline(const vk::raii::Device &device, const vk::raii::PipelineLayout &graphicsPipelineLayout, const vk::Format &colorFormat, const vk::Format &depthFormat, const vk::SampleCountFlagBits &msaaSamples);

            
};



