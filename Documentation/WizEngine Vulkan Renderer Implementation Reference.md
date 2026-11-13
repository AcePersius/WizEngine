# WizEngine Vulkan Renderer Implementation Reference

// To Update void VulkanEngine::loadGltf(const std::string &path, MeshData &meshData)
// update createTextureImage to reflect ktx2 changes and mip changes
// new overload for copyBufferToImage()
// new game object and changes to all things uniform buffer, descriptors, createdescriptorsets
// recordCommandBuffer
// Compute Family related stuff createLogicalDevice()
// createShaderStorageBuffers();
// createComputeDescriptorPool
// Shader Compute slang
// compute.slag and compute.spv as well as graphics.slang and graphics.spv
// updateComputeUniformBuffer
// draw frame

# Main.cpp:
    main()
    Purpose:
    Serve as the entry point of the application and coordinate the highest-level startup and shutdown of major systems.

    My Configuration:
    -Initialize the Logger.
        -The Logger writes runtime and error information to "latest.log".
        -The log file is cleared at the beginning of each application run.

    -Create and run the VulkanEngine.
        -VulkanEngine contains the Vulkan renderer and the application's current runtime loop.
        -The renderer is primarily based on the Vulkan.org Vulkan 1.4 tutorial.
        -Current renderer features:
            // List current Vulkan renderer features here.

    -After VulkanEngine finishes running, shut down the Logger.

    Why:
    main.cpp acts as the application's top-level coordinator rather than containing the implementation details of individual systems.

    The Logger and VulkanEngine each manage their own internal functionality, while main.cpp determines the order in which those systems are started, run, and shut down.

    This keeps the program entry point simple and prevents renderer-specific logic from being mixed directly into main.cpp.

    Important Consequences:
    -main() is the application's user-defined C++ entry point after the language/runtime startup work performed before main().
    -The Logger is available before VulkanEngine begins running.
    -The VulkanEngine controls its own initialization, main loop, rendering, and shutdown behavior.
    -Once VulkanEngine finishes, control returns to main.cpp and the Logger is shut down.
    -Future top-level engine systems can also be initialized and coordinated from main.cpp.

# VulkanEngine.hpp: 
    Purpose:
    Define the VulkanEngine class, including its public interface as well as the internal renderer state and function declarations required by its implementation.

    My Configuration:
    -Include Vulkan-Hpp and Vulkan RAII functionality.
        -Vulkan-Hpp provides C++-friendly Vulkan types, enums, methods, and struct initialization.
        -Vulkan RAII provides wrapper objects such as vk::raii::Device, Image, Pipeline, etc. that automatically destroy their underlying Vulkan resources when their lifetime ends.

    -Include SDL3.
        -SDL3 currently provides the application's window along with event and input handling.

    -Include required C++ standard-library functionality.
        -Currently includes containers such as std::vector.
        -Additional standard-library dependencies can be added as the renderer expands.

    -Include required application user-defined helper files.
        -Currently includes vulkan/vk_vertex.hpp.
        -Additional user-defined helper files can be added as the renderer expands.

    -Declare compile-time constants used by the renderer.
        -Individual constexpr values will be explained where they are actually used.

    -Declare the VulkanEngine class.

        Public Interface:
        -run()
            -Serves as the main public entry point into VulkanEngine.
            -Runs the major renderer/application stages in order:
                initWindow()
             -> initVulkan()
             -> mainLoop()
             -> cleanup()

        Private Renderer State:
        -SDL-related state such as the window.
        -Vulkan handles and objects required by the renderer.
        -Other renderer state such as flags, indices, formats, extents, vectors, and resource information.

        Private Function Declarations:
        -Initialization functions.
        -Resource creation/helper functions.
        -Rendering/runtime functions.
        -Swapchain management functions.
        -Cleanup and supporting functionality.

    Why:
    The header defines the structure and interface of VulkanEngine without containing the full implementation of every function.

    The public run() function gives the rest of the program a simple way to start the renderer, while Vulkan-specific implementation details remain private to VulkanEngine.

    Separating declarations into vk_engine.hpp and their implementations into vk_engine.cpp also keeps the class organization visible without requiring other parts of the program to know how each Vulkan subsystem is implemented.

    Important Consequences:
    -The rest of the application can interact with VulkanEngine through its public interface without directly managing Vulkan internals.
    -run() currently controls the VulkanEngine lifecycle from window initialization through the runtime loop and cleanup.
    -Renderer resources and state are encapsulated as private VulkanEngine members.
    -The header describes what state and functionality VulkanEngine possesses, while vk_engine.cpp provides the implementation of that functionality.
    -RAII Vulkan members automatically destroy their associated Vulkan resources when their wrapper objects are destroyed.

    
# VulkanEngine.cpp

## Application Lifecycle

### Block 1 — VulkanEngine.cpp Overview

    Purpose:
    Provide the implementation of the VulkanEngine class declared in VulkanEngine.hpp.

    My Configuration:
    -Include Vulkan-Hpp / Vulkan RAII functionality used by the renderer.
    -Include SDL3 functionality used for the application window and event handling.
    -Include supporting Vulkan headers where some renderer functionality has been separated into other files such as:
        -vk_device.hpp
        -vk_pipeline.hpp
        -vk_vertex.hpp
    -Include GLM for graphics-related mathematics such as:
        -vectors
        -matrices
        -matrix transformations
        -dot/cross products
    -Include C++ standard-library functionality required throughout the renderer.
    -Include/access the Logger interface used by this implementation; the Logger itself is initialized from main.cpp.
    -Define temporary model and texture file paths. // NOTE: This will be changed/removed in future versions of the project.
    -Define the application's current default window dimensions. // NOTE: This will be changed/removed in future versions of the project.
    -Define the requested Vulkan validation layer.
    -Use build configuration to determine whether validation layers should be enabled.

    Why:
    VulkanEngine.hpp declares what the VulkanEngine class contains, while VulkanEngine.cpp provides the implementation of those functions.

    Keeping the implementation in its own .cpp file prevents the class declaration from becoming filled with implementation details and allows Vulkan-related functionality to be further separated into supporting files when a subsystem becomes large enough to justify it.

    Validation layers are enabled during development because they provide diagnostic information about incorrect or suspicious Vulkan API usage. They are normally disabled in release builds because validation adds runtime overhead and is intended primarily as a development tool.

    Important Consequences:
    -VulkanEngine.cpp contains the primary implementation of the current renderer.
    -Supporting renderer systems can be moved into their own .hpp/.cpp files without changing VulkanEngine's public interface.
    -Development builds gain additional Vulkan error checking through validation layers.
    -Temporary model, texture, and window configuration currently remains directly associated with the renderer and can later be moved into more dedicated systems.

    Source Mapping:
    Original lines 90-103.
   

### Block 2 — `VulkanEngine::run()`

    Purpose:
    Control the high-level lifetime of VulkanEngine.

    My Configuration:
    run() calls the major application/renderer stages in this order:

        initWindow()
        -> initVulkan()
        -> mainLoop()
        -> cleanup()

    Why:
    The rest of the application should not need to know every individual step required to initialize or operate Vulkan.

    run() provides one simple public entry point that coordinates the renderer's entire current lifecycle.

    Important Consequences:
    -The SDL window is initialized before Vulkan objects depending on that window are created.
    -All Vulkan initialization occurs before entering the runtime loop.
    -mainLoop() owns normal application execution until shutdown is requested.
    -cleanup() runs after the application loop exits.
    -main.cpp can simply create/run VulkanEngine without directly managing Vulkan internals.

    Source Mapping:
    Original lines 105-111.


### Block 3 — `initWindow()`

    Purpose:
    Initialize SDL and create the operating-system window Vulkan will later render into.

    My Configuration:
    -Initialize the required SDL systems.
    -Create the application's SDL window using the configured initial width and height.
    -Store the resulting SDL_Window pointer in VulkanEngine.
    -Throw an error if initialization or window creation fails.

    Why:
    Vulkan itself does not create the normal desktop application window.

    SDL provides the platform-facing window that Vulkan will later connect to through a Vulkan surface.

    Important Consequences:
    -VulkanEngine now has a valid application window.
    -SDL can begin supplying window and input events to mainLoop().
    -The window can later be connected to Vulkan through createSurface().
    -Failure at this point prevents Vulkan initialization from continuing with an invalid window.

    Source Mapping:
    Original lines 108-109.
   

### Block 4 — `initVulkan()`

    Purpose:
    Coordinate creation of the Vulkan objects and resources required before normal rendering can begin.

    My Configuration:
    initVulkan() calls the renderer's initialization functions in dependency order.

    // FUTURE DEPENDENCY AUDIT:
    // Perform a complete dependency check when creating the Input/Output/Dependencies documentation
    // and/or the Obsidian architecture map. This should verify that every initialization function
    // occurs only after all resources and state it depends on have been created.

    Broadly, initialization builds/configures:
    -Vulkan instance and debugging support
    -Window surface
    -Physical GPU selection
    -MSAA sample-count capability selection
    -Logical device and graphics queue
    -Swapchain and image views
    -MSAA color and depth resources
    -Descriptor set layout
    -Graphics pipeline
    -Command infrastructure
    -Texture image, image view, and sampler
    -CPU-side model geometry
    -Vertex/index/uniform buffers
    -Descriptor pool and descriptor sets
    -Command buffers
    -Synchronization objects

    Why:
    Vulkan resources depend heavily on previously-created objects.

    For example:
    -A logical device requires a selected physical device.
    -A swapchain requires a surface and logical device.
    -Image resources require the device.
    -The graphics pipeline needs formats and pipeline layouts.
    -Descriptor sets require a descriptor pool and descriptor-set layout.

    initVulkan() therefore serves primarily as an initialization coordinator rather than implementing all of those systems itself.

    Important Consequences:
    -Initialization order reflects Vulkan resource dependencies.
    -By the time initVulkan() completes, VulkanEngine has the resources required to enter mainLoop() and begin drawing frames.
    -Individual Vulkan subsystems can be understood and maintained separately while initVulkan() keeps their startup order centralized.

    Source Mapping:
    Original lines 111-115 and the initialization sections that follow.
   

# Vulkan Foundation

### Block 5 — Vulkan Instance / `createInstance()`

    Purpose:
    Create the top-level Vulkan instance representing this application's connection to the Vulkan API.

    My Configuration:
    -Create a Vulkan-Hpp RAII Context.
    -Fill ApplicationInfo with:
        -application name
        -application version
        -engine name
        -engine version
        -requested Vulkan API version

    -Determine required instance layers.
        -Our development configuration requests the Vulkan validation layer.
        -Requested layers are checked against available instance layers before instance creation.

    -Determine required instance extensions using getRequiredInstanceExtensions().
        -SDL_Vulkan_GetInstanceExtensions() supplies extensions required to connect Vulkan to SDL's window system.
        -The Debug Utils extension is added when validation/debug functionality is enabled.

    -Fill InstanceCreateInfo with:
        -ApplicationInfo
        -requested layers
        -requested extensions

    -Create the vk::raii::Instance and store it in the instance member.

    Why:
    The Vulkan instance is the root Vulkan object used to interact with system-wide Vulkan functionality.

    Before creating it, the application must declare which instance-level extensions and layers it expects to use.

    SDL's required extensions vary depending on the platform, so querying SDL instead of hardcoding platform-specific extensions makes the application more portable.

    Important Consequences:
    -The application now has a Vulkan instance.
    -Available physical Vulkan devices can now be enumerated for later GPU suitability checking and selection.
    -A Vulkan surface can later be created for the SDL window.
    -Validation/debug functionality can be attached if its required layer/extensions are available.
    -Most later Vulkan initialization depends directly or indirectly on this instance.

    Source Mapping:
    Original lines 118-138.
   

### Block 6 — Validation and Debug Messaging / `setupDebugMessenger()`

    Purpose:
    Configure Vulkan validation/debug messages so incorrect, suspicious, or inefficient API usage can be reported during development.

    My Configuration:
    -If validation layers are disabled:
        -Return immediately without creating a debug messenger.

    -Create DebugUtilsMessengerCreateInfoEXT.

    -Enable message severities:
        -Warning
        -Error

    -Enable message types:
        -General
        -Performance
        -Validation

    -Provide the engine's Vulkan debug callback.

    The callback receives:
        -message severity
        -message type
        -VkDebugUtilsMessengerCallbackDataEXT
        -optional user data

    pCallbackData contains Vulkan-provided information about the reported message, including the validation/debug message string.

    pUserData is currently unused because no user-data pointer is assigned when the debug messenger is created.

    Current callback behavior:

        Warning
            -> print validation message to std::cerr
            -> send message to Logger using logLevel::Warning

        Error
            -> print validation message to std::cerr
            -> send message to Logger using logLevel::Error

    -Return vk::False from the callback.

    -Create the RAII debug messenger from the Vulkan instance.

    Related instance-extension configuration:
    -getRequiredInstanceExtensions() queries SDL for the extensions required by the current window system.
    -When validation layers are enabled, VK_EXT_debug_utils is additionally requested because the debug messenger functionality depends on that extension.

    Why:
    Vulkan deliberately performs relatively little defensive checking during normal execution.

    Validation layers provide an additional development-time layer that can detect many incorrect API usages, specification violations, and suspicious behavior before they become difficult rendering bugs or undefined behavior.

    The three enabled message types describe different categories of diagnostic information:

        General
            -> Events that do not cleanly belong to validation or performance categories.

        Performance
            -> Potentially inefficient Vulkan usage.

        Validation
            -> Vulkan specification violations or likely incorrect API usage.

    Warning and Error severities prevent less-important informational and verbose messages from being sent to the callback.

    The callback gives VulkanEngine control over what happens when one of these messages is generated. In the current implementation, warnings and errors are both printed to stderr and persisted through the Logger.

    Returning vk::False does NOT reset or destroy the debug messenger.

    It tells Vulkan that the callback is not requesting that the Vulkan call which triggered the message be aborted.

    VKAPI_ATTR and VKAPI_CALL ensure that the callback uses the function declaration/calling convention expected by Vulkan on the current platform.

    Important Consequences:
    -No debug messenger is created when validation layers are disabled.
    -Validation warnings and errors can be reported while the renderer is running.
    -General, Performance, and Validation message categories are all included in the callback filter.
    -Messages are visible immediately through stderr and are also preserved in the application's log file.
    -The debug messenger depends on VK_EXT_debug_utils being enabled at instance creation.
    -Returning vk::False allows normal execution to continue after the callback reports a message.
    -The debug messenger's Vulkan lifetime is managed through its RAII wrapper.

    Source Mapping:
    Original lines 140-160.
   

### Block 7 — Window Surface / `createSurface()`

    Purpose:
    Create the Vulkan surface that represents the SDL window as a presentation destination.

    My Configuration:
    -Create a raw VkSurfaceKHR initialized to VK_NULL_HANDLE.
    -Call SDL_Vulkan_CreateSurface() using:
        -SDL window
        -Vulkan instance
        -raw surface output

    -Wrap/store the resulting surface in the renderer's Vulkan RAII surface handle.

    Why:
    The SDL window and Vulkan instance exist independently.

    Vulkan needs a VkSurfaceKHR to ask questions such as:
    -Can this GPU present to this window?
    -What image formats can this surface display?
    -What presentation modes are supported?
    -What swapchain dimensions/image counts are supported?

    Important Consequences:
    -The application window is now represented inside Vulkan.
    -Physical-device selection can verify presentation support.
    -Swapchain capabilities can later be queried.
    -The swapchain will ultimately present its images through this surface.

    Source Mapping:
    Original lines 163-167.
   

# GPU Selection

### Block 8 — Physical Device Selection / `pickPhysicalDevice()`

    Purpose:
    Find a physical GPU that supports every capability required by WizEngine and choose the preferred compatible device.

    My Configuration:
    pickPhysicalDevice() calls selectPhysicalDevice(instance).

    selectPhysicalDevice():
    -Enumerate available Vulkan physical devices.
    -Evaluate every candidate with:
        -isDeviceSuitable()
        -scoreDevice()
    -Store suitable candidates ordered by score.
    -Return the highest-scoring suitable physical device.

    isDeviceSuitable():
    Checks required capabilities including:
    -Vulkan API version >= 1.3
    -at least one graphics-capable queue family
    -required device extensions:
        -VK_KHR_swapchain
    -sampler anisotropy
    -shaderDrawParameters
    -synchronization2
    -dynamicRendering
    -extendedDynamicState

    // SOURCE VERIFIED:
    // isDeviceSuitable() currently checks for a graphics-capable queue family,
    // but it does NOT check whether the physical device can present to the current surface.
    //
    // Presentation compatibility is checked later when
    // FindGraphicsPresentationQueueFamily() searches the already-selected physical device
    // for a queue family supporting both graphics and presentation.
    //
    // FUTURE ARCHITECTURE CHECK:
    // Consider moving presentation-support checking into physical-device suitability so that
    // selectPhysicalDevice() cannot select a GPU that satisfies the graphics/features checks
    // but cannot present to the application's Vulkan surface.

    scoreDevice():
    -Query physical-device properties.
    -Prefer discrete GPUs with a large score increase.
    -Add additional score based on hardware capability such as maximum supported image dimensions.

    Why:
    Vulkan does not automatically select the "best" GPU for the application.

    The engine must first determine whether a GPU is actually capable of running the renderer and then decide which compatible GPU should be preferred when multiple devices exist.

    Separating suitability from scoring is useful because:
    -Suitability answers: "Can this GPU run the renderer?"
    -Scoring answers: "Which compatible GPU would we rather use?"

    Important Consequences:
    -physicalDevice refers to a GPU that satisfies WizEngine's current Vulkan requirements.
    -A discrete GPU will normally be preferred over an integrated GPU when both satisfy the requirements.
    -Later renderer features can safely assume the capabilities checked here are supported.
    -Adding a new required Vulkan feature should generally also add a corresponding suitability check.
    -A future GPU selection menu could allow the user to override automatic scoring.

    Source Mapping:
    Original lines 169-205.
   


### Block 9 — Maximum MSAA Sample Count / `getMaxUsableSampleCount()`

    Purpose:
    Determine the highest multisample count supported by the selected GPU for both color and depth attachments.

    My Configuration:
    -Query physicalDevice properties.
    -Read:
        -framebufferColorSampleCounts
        -framebufferDepthSampleCounts

    -Find the bitwise intersection of the two supported sample-count sets.
    -Check supported sample counts from highest to lowest:
        64
        32
        16
        8
        4
        2
        1

    -Return the highest sample count supported by both color and depth attachments.
    -Store the result in msaaSamples.

    Why:
    Our multisampled color attachment and depth attachment must use compatible sample counts.

    A GPU may support different sample-count limits depending on the attachment type, so the renderer uses the highest value shared by both.

    Important Consequences:
    -msaaSamples becomes the sample count used by the graphics pipeline.
    -The MSAA color image and depth image must use this same sample count.
    -Texture images and swapchain images normally remain single-sampled.
    -The multisampled color image will eventually be resolved into a single-sample swapchain image.

    Source Mapping:
    Original lines 208-213.


# Logical Device and Queues

### Block 10 — Logical Device and Graphics Queue / `createLogicalDevice()`

    Purpose:
    Create the application's logical interface to the selected physical GPU, enable the Vulkan features and extensions required by WizEngine, and retrieve a queue capable of both graphics operations and presentation.

    My Configuration:

    findGraphicsPresentationQueueFamily():
    -Query the selected physical device's queue-family properties.
    -Iterate through each queue family.
    -For each queue family, check:
        -whether its queueFlags include Graphics capability
        -whether physicalDevice.getSurfaceSupportKHR() reports presentation support for the current Vulkan surface

    -Return the first queue-family index supporting BOTH graphics and presentation.
    -Store the returned index in graphicsIndex.
    -Throw an error if no suitable combined graphics/presentation queue family exists.

    makeLogicalDevice():

    Queue Request:
    -Create one DeviceQueueCreateInfo.
    -queueFamilyIndex = graphicsIndex
    -queueCount = 1
    -queue priority = 0.5f
        Queue priority is relative to other queues requested from the same queue family.
        Because WizEngine currently requests only one queue from this family, the exact
        priority value is not especially significant.

    Feature Chain:
    -Create a vk::StructureChain containing:
        -vk::PhysicalDeviceFeatures2
        -vk::PhysicalDeviceVulkan11Features
        -vk::PhysicalDeviceVulkan13Features
        -vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT

    -Enable:
        -samplerAnisotropy
        -shaderDrawParameters
        -synchronization2
        -dynamicRendering
        -extendedDynamicState

    DeviceCreateInfo:
    -Create vk::DeviceCreateInfo as the final description of what WizEngine requests from the selected physical device.
        -pNext = address of the PhysicalDeviceFeatures2 root of the feature chain
        -queueCreateInfoCount = 1
        -pQueueCreateInfos = address of deviceQueueCreateInfo
        -enabledExtensionCount = number of required device extensions
        -ppEnabledExtensionNames = requiredDeviceExtension.data()

    Current required device extension:
        -VK_KHR_swapchain

    Logical Device Creation:
    -Pass physicalDevice and deviceCreateInfo to vk::raii::Device.
    -Return the resulting logical device.
    -Store it in device.

    Graphics Queue Retrieval:
    -Pass device, graphics index, and queue index 0 from graphicsIndex.
    -Store the resulting queue in graphicsQueue.

    Why:
    A physical device describes an actual GPU and the features, extensions, limits, memory types, and queue families that GPU supports.

    The logical device is the application's Vulkan interface to that physical device.

    During logical-device creation, WizEngine must explicitly describe what parts of the GPU's available Vulkan functionality it intends to use.

    DeviceQueueCreateInfo answers: "Which queue family and how many queues do I want?"

    The feature chain answers: "Which optional/core Vulkan features do I want enabled?"

    The enabled device-extension list answers: "Which device-level extensions do I want enabled?"

    DeviceCreateInfo then combines these requests into the complete configuration passed to logical-device creation.

    The feature chain is connected through DeviceCreateInfo::pNext.

    vk::StructureChain manages the pNext links between the feature structures, while:
    featureChain.get<vk::PhysicalDeviceFeatures2>()
    provides the root structure that DeviceCreateInfo points to.

    It is therefore useful to think:
        Physical Device = "What can this GPU support?"

        DeviceCreateInfo = "Which supported capabilities does WizEngine request?"

        Logical Device = "Create WizEngine's Vulkan device interface using that requested configuration."

    The logical device is NOT a literal partition or subsection of the physical GPU.

    Important Consequences:
    -graphicsIndex identifies a queue family verified to support both graphics and presentation.

    -If the selected physical device has no queue family supporting both graphics and presentation, initialization fails at this stage.

    -device becomes the parent object used to create most later Vulkan resources.

    -The Vulkan features used later by WizEngine have been explicitly enabled during device creation.

    -VK_KHR_swapchain is enabled so swapchain functionality can be used by the logical device.

    -graphicsQueue provides the queue currently used for graphics submission and presentation.

    -Queue index 0 is used because WizEngine currently requests exactly one queue from the queue family selected with the graphicsIndex.

    -Command buffers allocated from a command pool associated with graphicsIndex are compatible with queues belonging to that queue family.

    -If WizEngine later separates graphics, compute, transfer, or presentation workloads, additional queue families or additional queues may be requested.

    Source Mapping:
    Original lines 215-254.
   


# Presentation and Swapchain

### Block 11 — Swapchain Overview

    Purpose:
    Define the presentation-image system WizEngine uses to render images that can later be presented through the application's Vulkan surface.

    Mental Model:
        Vulkan Surface
            ↓
        Swapchain
            ↓ owns/provides
        Presentable Images
            ↓
        acquireNextImage()
            ↓
        Render into acquired image
            ↓
        presentKHR()
            ↓
        Presentation system

    Why:
    A renderer should not attempt to render repeatedly into one image while that same image may still be involved in presentation.

    The swapchain provides a collection of presentable images whose availability is coordinated with the presentation system.

    During normal rendering:
    -The renderer acquires an available swapchain image.
    -Rendering targets that acquired image.
    -The completed image is submitted for presentation.
    -The presentation system eventually makes previously-used images available again.

    My Configuration:
    WizEngine currently prefers to request a minimum of three swapchain images when the surface allows it.

    This requested value is passed through SwapchainCreateInfoKHR::minImageCount.

    minImageCount is a minimum request, NOT a guarantee that the swapchain will contain exactly that many images.

    The actual images created by the implementation are retrieved later using:
        swapChain.getImages()

    Swapchain image count is separate from MAX_FRAMES_IN_FLIGHT.

    These answer different questions:
        Swapchain image count:
        = How many presentable images does this swapchain contain?

        MAX_FRAMES_IN_FLIGHT:
        = How many sets of reusable frame resources may be progressing through the CPU/GPU pipeline concurrently?

    Important Consequences:
    -The swapchain owns/manages the presentable images used for presentation.
    -WizEngine requests at least three images when surface limits permit it.
    -The implementation may create more images than the requested minimum.
    -The renderer must use the images actually returned by swapChain.getImages().
    -frameIndex does not identify a swapchain image.
    -imageIndex returned by acquireNextImage() identifies the swapchain image acquired for a particular rendering operation.

    Source Mapping:
    Original lines 256-261 and 292-297.

    Block 12 — Swapchain Capability Queries and Configuration Selection

    Purpose:
    Query what the selected physical device + Vulkan surface support, then choose the swapchain configuration WizEngine prefers within those limits.

    My Configuration:

    Query Phase — performed inside createSwapChain():

    Surface Formats:
        availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface)

    Presentation Modes:
        availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface)

    Surface Capabilities:
        surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface)

    These queries answer:
        "What configurations does this physical device support for this particular Vulkan surface?"

    Selection Phase:

    chooseSwapSurfaceFormat(availableFormats):

    -If availableFormats is empty:
        -throw an error.

    -Prefer:
        format = B8G8R8A8Srgb
        colorSpace = SrgbNonlinear

    -If the preferred combination exists:
        -return it.

    -Otherwise:
        -fall back to availableFormats[0].

    -Store the result in:
        swapChainSurfaceFormat

    Why:

    WizEngine prefers an sRGB swapchain format for normal color rendering, but the exact available combinations are surface/platform dependent.

    chooseSwapPresentMode(availablePresentModes):

    -Search for:
        Mailbox

    -If Mailbox is available:
        -return Mailbox.

    -Otherwise:
        -return FIFO.

    Why:
    Mailbox is WizEngine's preferred current presentation behavior.

    FIFO provides the fallback presentation mode.

    chooseSwapExtent(surfaceCapabilities, window):

    -Check surfaceCapabilities.currentExtent.

    -If currentExtent.width is NOT UINT32_MAX:
        -the presentation system has already specified the swapchain extent.
        -return currentExtent directly.

    -Otherwise:
        -query the SDL window size in pixels using SDL_GetWindowSizeInPixels().
        -throw an error if SDL cannot provide the size.
        -clamp the width between:
            minImageExtent.width
            maxImageExtent.width
        -clamp the height between:
            minImageExtent.height
            maxImageExtent.height
        -return the resulting Extent2D.

    -Store the result in:
        swapChainExtent

    Why:
    Swapchain images are measured in pixels.

    Some window systems dictate the required extent directly through currentExtent.

    Others allow the application to select an extent, in which case WizEngine uses the SDL window's pixel dimensions while remaining within the surface's supported limits.

    chooseSwapMinImageCount(surfaceCapabilities):

    -Start with:
        max(3, surfaceCapabilities.minImageCount)

    -If: surfaceCapabilities.maxImageCount > 0 AND the requested count exceeds that maximum:
        -clamp the request to maxImageCount.
    -Return the resulting minimum-image request.

    Important detail:
        maxImageCount == 0
    means the surface does not advertise a fixed maximum image count.

    Why:
    WizEngine currently prefers at least three swapchain images when possible while still obeying the minimum and maximum image-count limits reported by the surface.

    Important Consequences:
    -Swapchain configuration adapts to the selected GPU/surface combination rather than assuming one configuration is universally supported.
    -WizEngine prefers B8G8R8A8Srgb + SrgbNonlinear when available.
    -WizEngine prefers Mailbox and falls back to FIFO.
    -swapChainExtent respects the window system's extent requirements.
    -WizEngine requests at least three images when surface limits permit it.
    -The chosen surface format and extent are retained because later rendering resources depend on them.

    Source Mapping:
    Original lines 263-297.

    Block 13 — Swapchain Creation and Image Retrieval / createSwapChain()

    Purpose:
    Combine the selected surface-compatible configuration into SwapchainCreateInfoKHR, create the swapchain, and retrieve the actual presentable images created for it.

    My Configuration:

    After performing the capability queries and configuration selections described in Block 12, create:
        vk::SwapchainCreateInfoKHR

    Configuration:

    surface = *surface
        -The Vulkan surface this swapchain will present to.

    minImageCount = minImageCount
        -The minimum number of presentable images WizEngine requests.
        -The implementation may create more than this number.

    imageFormat = swapChainSurfaceFormat.format
        -The selected pixel format of the swapchain images.

    imageColorSpace = swapChainSurfaceFormat.colorSpace
        -The selected interpretation/color-space behavior for those image values.

    imageExtent = swapChainExtent
        -The width and height in pixels of every swapchain image.

    imageArrayLayers = 1
        -Each swapchain image contains one array layer.
        -This does NOT mean "one image per frame."

    imageUsage = ColorAttachment
        -WizEngine intends to use the swapchain images as color attachments.
        -In the current MSAA renderer, the acquired swapchain image acts as the single-sample resolve destination.

    imageSharingMode = Exclusive
        -The swapchain images use exclusive queue-family ownership rather than concurrent sharing.
        -This matches the current architecture because Block 10 verifies that graphics and presentation are handled by the same queue family.

    preTransform = surfaceCapabilities.currentTransform
        -Use the surface's current transformation.

    compositeAlpha = Opaque
        -Treat the window as opaque for composition with the window system.

    presentMode = presentMode
        -Use the presentation mode selected in Block 12.

    clipped = true
        -The application does not require predictable contents for regions of the window that are obscured by other windows.

    oldSwapchain:
        -Not explicitly assigned in the current initializer.
        -It therefore remains at its default/null value in the current creation path.

    Create:
        swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo)

    Then retrieve the images actually created by the swapchain:

        swapChainImages = swapChain.getImages()

    Why:
    Blocks 11–13 can be viewed as three separate questions:

        Block 11:
        "What is the swapchain doing for the renderer?"

        Block 12:
        "What does this surface support, and which supported options does WizEngine prefer?"

        Block 13:
        "Now that those choices have been made, how do we create the actual swapchain?"

    Creating SwapchainKHR creates/manages the swapchain's presentable images.

    swapChain.getImages() does NOT create those images.

    It retrieves vk::Image handles referring to the images provided by the newly-created swapchain.

    Unlike normal engine-created images, WizEngine does not separately create these images or allocate/bind DeviceMemory for them.

    Important Consequences:
    -swapChain represents the presentation image chain associated with the Vulkan surface.
    -swapChainImages contains the actual image handles returned by that swapchain.
    -swapChainImages.size() represents the actual number of images obtained, which can exceed minImageCount.
    -The swapchain images use swapChainSurfaceFormat and swapChainExtent.
    -The images can be used as rendering color attachments and later presented.
    -Later image views are created for these swapchain images in Block 15.
    -Resources derived from swapchain format, extent, or image count may need recreation when the swapchain is recreated.

    // FUTURE PORTABILITY CHECK:
    // The current source hardcodes CompositeAlpha::eOpaque.
    // VkSurfaceCapabilitiesKHR exposes supportedCompositeAlpha, so a more defensive implementation
    // could verify that eOpaque is supported before requesting it.
    //
    // The current source also requests ImageUsageFlagBits::eColorAttachment directly.
    // supportedUsageFlags in VkSurfaceCapabilitiesKHR describes which image usages are supported.
    // This is worth checking if the renderer is later made more portability-focused or uses
    // additional swapchain-image usages.

    Source Mapping:
    Original lines 299-327.


# Vulkan Image Resources

### Block 14 — Vulkan Image Model

    Purpose:
    Establish the distinction between Vulkan memory, images, and image views used throughout the renderer.

    My Configuration / Mental Model:

        DeviceMemory
            ↓ backs
        vk::Image
            ↓ interpreted/accessed through
        vk::ImageView

    DeviceMemory:
    -Represents a Vulkan memory allocation.

    Image:
    -Represents an image resource with properties such as:
        -dimensions
        -format
        -mip levels
        -array layers
        -sample count
        -tiling
        -usage

    ImageView:
    -Defines how a particular use interprets an image.
    -Can select:
        -format
        -aspect
        -mip range
        -array-layer range
        -view type

    Why:
    A vk::Image is not itself "the image memory."

    The image is a Vulkan resource that must normally be bound to a compatible DeviceMemory allocation.

    An ImageView then describes which portion/interpretation of that image is exposed to rendering or shaders.

    Important Consequences:
    -Multiple image views can reference the same underlying image.
    -The lifetime of an image's bound DeviceMemory must cover the image's use.
    -Destroying an ImageView does not destroy the underlying Image.
    -Different renderer resources such as textures, depth targets, MSAA targets, and swapchain images all use the same basic image abstraction for different purposes.

    Source Mapping:
    This corrects and restructures concepts originally discussed around image-view and image-memory creation.   


### Block 15 — Swapchain Image Views / createImageViews() + createImageView()

    Purpose:
    Create Vulkan ImageViews that expose the swapchain images in a form that can be used by later rendering operations.

    Mental Model:
        swapChainImages
            ↓
        one ImageView per image
            ↓
        swapChainImageViews
            ↓
        usable as rendering attachment views

    createImageViews():

    -Clear swapChainImageViews.
        swapChainImageViews.clear()

        Because the vector stores vk::raii::ImageView objects, clearing the vector destroys
        the existing ImageView wrappers and releases their underlying Vulkan image views.

        This is especially important when image views are recreated after swapchain recreation.

    -Reserve enough vector capacity for one ImageView per swapchain image.
        swapChainImageViews.reserve(swapChainImages.size())

        reserve() only allocates enough vector capacity to avoid unnecessary reallocations.
        It does NOT:
        -create any ImageViews
        -change the vector's logical size
        -populate any elements

    -Iterate through every image in swapChainImages.

    -For each swapchain image, call:
        createImageView(image, swapChainSurfaceFormat.format, vk::ImageAspectFlagBits::eColor, 1)

    -Append the returned RAII ImageView to swapChainImageViews.

    Current swapchain-image configuration:
        image = current swapchain image
        format = swapChainSurfaceFormat.format
        aspectFlags = Color
        mipLevels = 1

    createImageView():

    Purpose:
    Provide a reusable helper for creating a 2D ImageView from an existing Vulkan Image.

    Inputs:
        image = the vk::Image that the view will reference
        format = how the image data should be interpreted by this view
        aspectFlags = which image aspect the view exposes
        mipLevels = how many mip levels beginning at mip level 0 are exposed

    ImageViewCreateInfo:
        .image = image
            -References the existing Vulkan Image.
            -The ImageView does not create or copy the underlying image data.
        .viewType = e2D
            -The current helper always creates a two-dimensional image view.
        .format = format
            -Specifies the format used to interpret the image through this view.
        .subresourceRange.aspectMask = aspectFlags
            -Selects which aspect of the image is exposed.

            Examples used in the renderer include: 
            Color, Depth

        .subresourceRange.baseMipLevel = 0
            -The view always begins at mip level 0.

        .subresourceRange.levelCount = mipLevels
            -Controls how many consecutive mip levels are visible through the view.

            For swapchain images:
                levelCount = 1

            For texture images:
                levelCount can include the full mip chain.

        .subresourceRange.baseArrayLayer = 0
            -The view begins at array layer 0.

        .subresourceRange.layerCount = 1
            -The current helper exposes exactly one array layer.

    -Create and return:
        vk::raii::ImageView(device, viewInfo)

    Why:
    A vk::Image represents the image resource itself, but many Vulkan operations require an ImageView rather than only the raw Image handle.

    The ImageView defines the particular interpretation/subresource range through which that image will be used.

    For the swapchain, every presentable image therefore receives its own corresponding ImageView:
        swapChainImages[0]          swapChainImages[1]          swapChainImages[2]
                ↓                           ↓                           ↓
        swapChainImageViews[0]      swapChainImageViews[1]      swapChainImageViews[2]

    Later, when a particular swapchain image is acquired using imageIndex, the renderer can use:
        swapChainImageViews[imageIndex]
    as the view corresponding to that acquired image.

    The helper is reusable because color, depth, and texture images all need ImageViews, while differing in values such as:
    -format
    -aspect
    -number of visible mip levels

    However, createImageView() is not fully general-purpose.
        It currently assumes:
            -2D image views
            -base mip level 0
            -base array layer 0
            -exactly one array layer
        Supporting resources such as:
            -texture arrays
            -cubemaps
            -multi-layer images
        would likely require expanding the helper or creating specialized ImageView creation functions.

    Important Consequences:
    -There is one swapchain ImageView for every swapchain Image.
    -The views use the same format selected for the swapchain.
    -Swapchain views expose only the color aspect.
    -Swapchain views expose exactly one mip level.
    -swapChainImageViews can later be indexed using the imageIndex returned by acquireNextImage().
    -ImageViews do not own or duplicate the underlying image contents.
    -The RAII ImageViews are automatically destroyed when removed from the vector or when their C++ lifetime ends.
    -createImageView() is shared by several renderer image-resource types while still having current limitations to 2D, single-layer views.

    Source Mapping:
    Original lines 329-349.


### Block 16 — Generic Image Allocation / createImage() + findMemoryType()

    Purpose:
    Create an engine-owned 2D Vulkan Image, find a compatible physical-device memory type, allocate DeviceMemory for the image, bind the allocation to the image, and return both RAII resources together.

    Mental Model:
        describe image
            ↓
        create vk::Image
            ↓
        ask Vulkan for memory requirements
            ↓
        find compatible memory type
            ↓
        allocate DeviceMemory
            ↓
        bind memory to image
            ↓
        return Image + DeviceMemory

    createImage():

    Inputs:
        width / height = image dimensions

        mipLevels = number of mip levels the image contains

        numSamples = number of samples stored per pixel

        format = image pixel/depth format

        tiling = how Vulkan arranges the image data in memory

        usage = operations the image is allowed to participate in

        properties = memory-property flags required from the backing memory

    ImageCreateInfo:
        imageType = e2D
            -The helper currently creates two-dimensional images.

        format = format

        extent = width, height, 1 (depth)
            -A 2D image still uses the three-component VkExtent3D structure.
            -Depth is therefore set to 1.

        mipLevels = mipLevels

        arrayLayers = 1
            -The current helper creates a single-layer image.

        samples = numSamples
            -Allows the same helper to create either:
                single-sample images or multisampled images

        tiling = tiling

        usage = usage
            -The caller defines how the image will be used.

            Examples elsewhere in WizEngine include:
                ColorAttachment
                DepthStencilAttachment
                TransferSrc
                TransferDst
                Sampled

        sharingMode = Exclusive
            -The image uses exclusive queue-family ownership.

    Create the Image:
        vk::raii::Image image{device, imageInfo}

    At this point, the Vulkan Image object exists, but it does not yet have backing DeviceMemory bound to it.

    Query Memory Requirements:
        memRequirements = image.getMemoryRequirements()

    Vulkan returns requirements for this particular image resource.

    Relevant fields include:
        size = minimum allocation size required for the image

        alignment = required alignment if the resource is bound at an offset

        memoryTypeBits = bitmask identifying which physical-device memory types are compatible with this image

    MemoryAllocateInfo:
        allocationSize = memRequirements.size

        memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)

    Create:
        vk::raii::DeviceMemory imageMemory{device, allocInfo}

    Bind Memory:
        image.bindMemory(*imageMemory, 0)

    The image is bound to imageMemory beginning at offset 0.

    Because the image occupies its own allocation in the current implementation, there is no need to calculate a suballocation offset.

    Offset 0 also naturally satisfies the allocation's required alignment.

    Return:

        return{std::move(image), std::move(imageMemory)}

    vk::raii resources represent ownership of Vulkan objects and are move-oriented rather than copied.

    std::move() transfers ownership of the newly-created Image and DeviceMemory into the returned pair without attempting to duplicate the Vulkan resources.

    findMemoryType():

    Purpose:
    Find a physical-device memory type that is both compatible with the resource and satisfies the memory properties requested by the caller.

    Query:
        physicalDevice.getMemoryProperties()

    This returns the physical device's available memory types and memory heaps.

    Iterate through:
        memProperties.memoryTypes[0 ... memoryTypeCount]

    Compatibility Check #1:
        typeFilter & (1 << i)

    typeFilter is the memoryTypeBits value returned by Vulkan for the resource.

    Each bit corresponds to one physical-device memory type.

    If bit i is set:
        memory type i is compatible with this particular resource.


    Compatibility Check #2:
        (memoryTypes[i].propertyFlags & properties) == properties

    This checks whether memory type i contains ALL memory-property flags requested by the caller.

    For example, if the caller requests:
        HostVisible | HostCoherent
    the chosen memory type must contain both flags.

    The memory type is allowed to contain additional flags as well.

    If both conditions are true:
        return i

    Otherwise continue searching.

    If no compatible memory type is found:
        throw runtime_error

    Why:
    Vulkan deliberately separates resource creation from memory allocation.

    Creating vk::Image defines the image resource:
        dimensions
        format
        usage
        sample count
        mip structure
        etc.

    but it does not automatically provide the DeviceMemory that stores that image's backing data.

    Vulkan must first tell the application what memory is compatible with the resource.

    WizEngine then chooses a memory type satisfying both:
        Vulkan's compatibility requirements AND WizEngine's requested memory properties

    This allows the same createImage() helper to support several different image-resource types.

    For example:
        textureImage
            -> single sampled
            -> TransferSrc | TransferDst | Sampled
            -> DeviceLocal

        colorImage
            -> multisampled
            -> ColorAttachment
            -> DeviceLocal

        depthImage
            -> multisampled
            -> DepthStencilAttachment
            -> DeviceLocal

    Important Consequences:
    -The Image resource and its DeviceMemory allocation are separate Vulkan objects.

    -Creating the Image alone is not sufficient; compatible DeviceMemory must also be allocated and bound.

    -memoryTypeBits determines which physical-device memory types Vulkan permits for that specific resource.

    -The properties argument determines which characteristics WizEngine requires from the chosen compatible memory type.

    -findMemoryType() currently returns the first memory type satisfying both conditions rather than ranking all compatible memory types.

    -createImage() currently creates a separate DeviceMemory allocation for each image it creates.

    -The image is bound at memory offset 0 because the current implementation does not suballocate multiple resources inside a larger allocation.

    -The returned Image and DeviceMemory must both remain alive while the image is being used.

    -The same helper supports texture, depth, and MSAA color images by changing its input parameters.

    // FUTURE MEMORY-ALLOCATION NOTE:
    // This helper currently performs one Vulkan memory allocation per image.
    // That is straightforward and appropriate for the renderer's current scale,
    // but larger engines commonly use suballocation or a dedicated allocator such as VMA
    // to reduce the number of Vulkan memory allocations and manage memory more efficiently.

    Source Mapping:
    Original lines 363-387.


### Block 17 — MSAA Color Resource / `createColorResources()`

    Purpose:
    Create the multisampled color attachment used as WizEngine's primary color rendering target when MSAA is enabled.

    My Configuration:
    -colorFormat = swapChainSurfaceFormat.format

    Call createImage() with:
    -width = swapchain width
    -height = swapchain height
    -mipLevels = 1
    -samples = msaaSamples
    -format = colorFormat
    -tiling = Optimal
    -usage:
        -TransientAttachment
        -ColorAttachment
    -memory = DeviceLocal

    Store:
    -colorImage = std::move(image)
    -colorImageMemory = std::move(imageMemory)

    Create colorImageView using createImageView():
    -image = colorImage
    -format = colorFormat
    -aspect = Color
    -mipLevels = 1

    This creates a 2D ImageView of the MSAA color image using the same color format as the swapchain and exposing its single color mip level.

    Why:
    Swapchain images are single-sampled presentable images.

    With MSAA enabled, the renderer needs a multisampled color target containing multiple samples per pixel.

    colorImageView allows colorImage to later be used as the multisampled color attachment during dynamic rendering.

    Rendering therefore happens into:
        MSAA colorImage
            ↓ accessed through colorImageView
            ↓ resolve
        single-sample swapchain image
            ↓
        presentation

    Without MSAA or another intermediate rendering requirement, a separate colorImage may not be necessary because rendering could occur directly into the swapchain image.

    Important Consequences:
    -colorImage uses the same color format and dimensions as the swapchain.
    -colorImage uses msaaSamples rather than one sample per pixel.
    -colorImageView exposes colorImage as a color attachment with one mip level.
    -colorImage is an intermediate render target and is not itself presented.
    -Its rendered result must be resolved into the acquired swapchain image.
    -colorImage and colorImageView are swapchain-dependent and must be recreated when relevant swapchain properties change.

    Source Mapping:
    Original lines 351-390.


### Block 18 — Depth Resources / `createDepthResources()`

    Purpose:
    Create the depth attachment used to determine which fragments are visible when geometry overlaps on screen.

    My Configuration:

    Depth-format selection:
    -Provide several acceptable depth formats.
    -Call findSupportedFormat() using:
        -candidate formats
        -Optimal tiling
        -DepthStencilAttachment feature requirement

    findSupportedFormat():
    -Query format properties for each candidate.
    -Check whether the requested tiling mode supports the required format features.
    -Return the first compatible format.

    Create depth-image using createImage():
    -swapchain width/height.
    -mipLevels = 1
    -samples = msaaSamples
    -format = selected depth format
    -tiling = Optimal
    -usage = DepthStencilAttachment
    -memory = DeviceLocal

    Store:
    -depthImage = std::move(image)
    -depthImageMemory = std::move(imageMemory)

    Create depthImageView using createImageView():
    -image = depthImage
    -format = selected depth format
    -aspect = Depth
    -mipLevels = 1

    This creates a 2D ImageView of the depth image that exposes its depth aspect and single mip level for use as the renderer's depth attachment.

    Why:
    For opaque geometry, Vulkan needs a way to determine whether a newly-generated fragment is closer to the viewer than the fragment currently stored at that screen location.

    The depth image stores depth-buffer values produced after the graphics pipeline's coordinate transformations.

    It is NOT simply a storage image containing an object's world-space Z coordinate.

    Because our color target is multisampled, the depth attachment must use a compatible multisample count.

    depthImageView allows the depthImage to later be supplied to dynamic rendering as the depth attachment.

    Important Consequences:
    -The renderer can perform depth testing.
    -Closer opaque fragments can prevent farther fragments from overwriting them.
    -The depth resource uses a GPU-supported depth format rather than assuming one format works everywhere.
    -depthImage uses msaaSamples so its sample count matches the multisampled color attachment.
    -depthImageView exposes the depth aspect of depthImage with one mip level.
    -depthImage and depthImageView are swapchain-sized and therefore swapchain-dependent.
    -The depth attachment is used during rendering but is not presented to the screen.

    Source Mapping:
    Original lines 393-418.
   

# Vulkan Buffer Resources

### Block 19 — Generic Buffer Allocation / `createBuffer()`

    Purpose:
    Create a Vulkan Buffer, allocate compatible DeviceMemory for it, bind the memory to the buffer, and return both resources.

    My Configuration:

    createBuffer() inputs:
    -size
        -Size of the buffer in bytes.
    -usage
        -Defines how the buffer will be used.
        -Examples:
            -TransferSrc
            -TransferDst
            -VertexBuffer
            -IndexBuffer
            -UniformBuffer
    -properties
        -Required properties of the DeviceMemory that will back the buffer.
        -Examples:
            -DeviceLocal
            -HostVisible
            -HostCoherent
    Fill BufferCreateInfo:
    -size = requested size
    -usage = requested usage flags
    -sharingMode = Exclusive

    Create:
    -vk::raii::Buffer using device and bufferInfo

    Query the buffer's memory requirements:
    -memRequirements = buffer.getMemoryRequirements()

    Fill MemoryAllocateInfo:
    -allocationSize = memRequirements.size
        -Allocates enough DeviceMemory to satisfy the buffer's required size.

    -memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
        -memoryTypeBits limits the search to memory types Vulkan says are compatible with this buffer.
        -properties contains the memory characteristics requested by the caller.
        -findMemoryType() returns a memory type satisfying both requirements.

    Allocate:
    -bufferMemory = vk::raii::DeviceMemory(device, allocInfo)

    Bind:
    -buffer.bindMemory(*bufferMemory, 0)
        -Binds the allocated DeviceMemory to the buffer starting at offset 0.

    Return:
    -return {std::move(buffer), std::move(bufferMemory)}

    Why:
    Vulkan separates a Buffer resource from the DeviceMemory that backs it.

    Creating the Buffer defines properties such as:
    -size
    -usage
    -sharing mode

    The renderer must then:
    -query that buffer's memory requirements
    -find a compatible memory type
    -allocate DeviceMemory
    -bind that memory to the buffer

    The properties argument applies to the backing DeviceMemory rather than the Buffer itself.

    This allows the same createBuffer() helper to create buffers with different memory requirements.

    For example:
    -staging buffers can request HostVisible | HostCoherent memory.
    -vertex/index destination buffers can request DeviceLocal memory.
    -uniform buffers can request HostVisible | HostCoherent memory.

    Important Consequences:
    -The Buffer and its DeviceMemory are separate Vulkan objects.
    -The Buffer cannot be used normally until compatible memory has been allocated and bound.
    -memRequirements.size determines how much DeviceMemory is allocated.
    -memRequirements.memoryTypeBits determines which physical-device memory types are valid for that buffer.
    -findMemoryType() combines Vulkan's compatible memory types with the memory properties requested by WizEngine.
    -The memory is bound at offset 0 because the current implementation gives the buffer its own allocation rather than suballocating from a larger memory block.
    -The Buffer and DeviceMemory must both remain alive while the resource is in use.
    -The same helper is reused for staging, vertex, index, and uniform buffers.

    Source Mapping:
    The original documentation referenced createBuffer() from texture, vertex, index, and uniform-buffer creation, but did not separately document the helper itself.
    Relevant original references include lines 613-617, 721-725, 739-743, and 757-762.


# Descriptor System

### Block 20 — Descriptor System Mental Model

    Purpose:
    Define how shaders are connected to GPU resources such as uniform buffers and textures.

    My Configuration / Mental Model:

    Descriptor Set Layout:
    Defines the binding contract expected by shaders and the pipeline.

    Each binding can specify:
    -binding number
    -descriptor type
    -descriptor count
    -shader stages allowed to access it

    Example:
        binding 0 -> Uniform Buffer
        binding 1 -> Combined Image Sampler

    The layout defines what resources are expected, but does not contain the actual resources.

    Descriptor Pool:
    Allocator/capacity used to allocate descriptor sets and their descriptors.

    It does not contain the actual buffer or image data referenced by those descriptors.

    Descriptor Set:
    A concrete set of bindings populated with references to actual resources.

    Pipeline Layout:
    Defines which descriptor-set layouts and push-constant ranges are compatible with the pipeline.

    Current resource relationship:
        DescriptorSetLayout
            ↓
        DescriptorSet[frame]
        ├── binding 0 -> uniformBuffers[frame]
        └── binding 1 -> textureImageView + textureSampler

    The Combined Image Sampler binding therefore connects both:
    -the ImageView describing the texture image
    -the Sampler describing how that texture should be sampled

    Why:
    Shaders need a standardized mechanism for accessing resources that are not supplied through the dedicated vertex/index-buffer paths.

    A descriptor is better understood as shader-visible resource binding information, not literally as a user-facing pointer to raw GPU memory.

    The layout defines what kinds of resources are expected.

    The descriptor set specifies which actual resources satisfy those bindings.

    Important Consequences:
    -Shaders can access the current frame's uniform-buffer data.
    -Shaders can sample the current texture.
    -Descriptor-set compatibility is established through the descriptor-set layout and pipeline layout.
    -Descriptor pools allocate descriptor sets but do not own the underlying buffer/texture data referenced by them.
    -Vertex/index buffers are not passed through this descriptor system in the current renderer.
    -Future resources such as storage buffers or additional textures can be added through additional descriptor bindings.

    Source Mapping:
    Corrected from the original descriptor overview and later descriptor sections.
   

### Block 21 — Descriptor Set Layout / `createDescriptorSetLayout()`

    Purpose:
    Define the descriptor-binding interface that shaders and later descriptor sets are expected to follow.

    My Configuration:
    Create an array containing two DescriptorSetLayoutBinding entries.

    Binding 0:
    -binding = 0
    -descriptorType = UniformBuffer
    -descriptorCount = 1
    -stageFlags = Vertex

    This defines one UniformBuffer descriptor at binding 0 that can be accessed by the vertex shader.

    Binding 1:
    -binding = 1
    -descriptorType = CombinedImageSampler
    -descriptorCount = 1
    -stageFlags = Fragment

    This defines one CombinedImageSampler descriptor at binding 1 that can be accessed by the fragment shader.

    descriptorCount = 1 means each binding contains one descriptor element rather than an array of multiple descriptors.

    Fill DescriptorSetLayoutCreateInfo:
    -bindingCount = bindings.size()
    -pBindings = bindings.data()

    Create/store:
    -descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo)

    Why:
    The pipeline needs to know the expected shader-resource interface before concrete descriptor sets are allocated and populated.

    Binding 0 establishes:
        UniformBuffer -> vertex shader
    Binding 1 establishes:
        CombinedImageSampler -> fragment shader

    The CombinedImageSampler combines:
    -an ImageView identifying the sampled image
    -a Sampler describing how that image is sampled

    It has nothing inherently to do with MSAA.

    stageFlags restrict which shader stages are permitted to access each descriptor binding.

    The binding numbers 0 and 1 identify locations inside this DescriptorSetLayout.
    The descriptor-set number itself is established later when this layout is included in the PipelineLayout.

    Important Consequences:
    -Shaders and descriptor sets must agree with this binding contract.
    -Binding 0 expects one UniformBuffer descriptor accessible from the vertex shader.
    -Binding 1 expects one CombinedImageSampler descriptor accessible from the fragment shader.
    -The layout defines descriptor types and locations but does not yet reference concrete uniform-buffer or texture resources.
    -The layout can be used to create the PipelineLayout before concrete descriptor sets exist.
    -Changing this shader-resource interface may require compatible changes to shaders, descriptor writes, and pipeline-layout configuration.

    Source Mapping:
    Original lines 430-442.


# Graphics Pipeline

### Block 22 — Graphics Pipeline Overview / `createGraphicsPipeline()`

    Purpose:
    Create the graphics pipeline that defines how vertex data, shaders, rasterization, depth testing, multisampling, and color output are processed during rendering.

    My Configuration:
    createGraphicsPipeline():
    -Create pipelineLayout using makePipelineLayout().
    -Select a supported depth format using findSupportedFormat().
    -Create graphicsPipeline using makeGraphicsPipeline().

    makeGraphicsPipeline() receives:
    -logical device
    -pipeline layout
    -swapchain color format
    -selected depth format
    -MSAA sample count

    The pipeline contains configuration for:
    -shader stages
    -vertex input
    -input assembly
    -viewport/scissor state
    -rasterization
    -multisampling
    -depth/stencil testing
    -color blending
    -dynamic state
    -pipeline layout
    -dynamic rendering attachment formats

    Why:
    Vulkan requires most graphics-pipeline behavior to be explicitly described before drawing.

    Breaking the pipeline configuration into smaller state sections makes an otherwise large object much easier to understand.

    Selecting the depth format before pipeline creation ensures the pipeline is configured for a depth-attachment format supported by the physical device.

    Important Consequences:
    -graphicsPipeline becomes the primary rendering pipeline bound during draw commands.
    -Its state determines how our current mesh geometry reaches the final color/depth attachments.
    -The pipeline's color format matches the swapchain format.
    -The pipeline's depth format is selected from GPU-supported depth formats.
    -Changes to non-dynamic pipeline state generally require creation of another compatible pipeline.

    Source Mapping:
    Original lines 446-460 and 573-577.
   

### Block 23 — Pipeline Layout / `makePipelineLayout()`

    Purpose:
    Define the shader-resource interface expected by the graphics pipeline.

    My Configuration:
    PipelineLayoutCreateInfo:
    -setLayoutCount = 1
    -pSetLayouts = descriptorSetLayout
    -pushConstantRangeCount = 0

    Because descriptorSetLayout is the only DescriptorSetLayout supplied, it occupies:
    -set = 0

    Create and return vk::raii::PipelineLayout.

    Why:
    The graphics pipeline needs to know which descriptor-set layouts and push-constant ranges are compatible with shader-resource access.

    The PipelineLayout does NOT contain the actual UBO or texture resources.

    Instead, it describes the layout/interface those resources must follow when descriptor sets are bound later.

    Important Consequences:
    -The graphics pipeline becomes compatible with descriptor sets created from descriptorSetLayout.
    -descriptorSetLayout occupies descriptor set 0 in the current PipelineLayout.
    -Concrete descriptor sets can be allocated later.
    -Push constants are currently not used but can later be added to this pipeline-layout interface.

    Source Mapping:
    Original lines 451-454 and the later pipeline-layout assignment.


### Block 24 — Shader Modules and Shader Stages

    Purpose:
    Load compiled shader code, create a Vulkan shader module, and describe which shader entry points belong to the graphics pipeline.

    My Configuration:
    Shader source:
    -Shaders are written in Slang.
    -They are compiled externally into SPIR-V binary code.
    -Current compiled shader file = shaders/slang.spv

    readfile():
    -Open the compiled SPIR-V file as binary.
    -Read its bytes into a char vector.

    createShaderModule():
    -Create ShaderModuleCreateInfo.
    -codeSize = compiled byte count
    -pCode = binary data reinterpreted as uint32_t words
    -Create/return ShaderModule.

    One shaderModule is created from slang.spv.

    Shader stages:

    Vertex:
    -stage = Vertex
    -module = shaderModule
    -pName = "vertMain"

    Fragment:
    -stage = Fragment
    -module = shaderModule
    -pName = "fragMain"

    Combine both into the pipeline's shader-stage array.

    Why:
    Vulkan does not compile high-level Slang source at normal pipeline creation time.

    The shader is compiled to SPIR-V first, and Vulkan receives that compiled representation through a ShaderModule.

    Both current shader stages use the same SPIR-V module, while pName selects the specific entry-point function used for each stage.

    Important Consequences:
    -The graphics pipeline executes vertMain as its vertex shader.
    -The graphics pipeline executes fragMain as its fragment shader.
    -Both stages currently reference the same shaderModule.
    -One SPIR-V module can contain multiple entry points.
    -Changing shader source requires recompiling the SPIR-V and creating/recreating a graphics pipeline that uses the updated shader module.
    -Once pipeline creation has completed successfully, the ShaderModule used to create it does not need to remain alive for later draw calls.

    Source Mapping:
    Original lines 463-487.


### Block 25 — Vertex Input State

    Purpose:
    Describe how bytes from the bound vertex buffer are interpreted as shader vertex attributes.

    My Configuration:
    Obtain:
    -Vertex::getBindingDescription()
    -Vertex::getAttributeDescriptions()

    PipelineVertexInputStateCreateInfo:
    -vertexBindingDescriptionCount = 1
    -pVertexBindingDescriptions = bindingDescription
    -vertexAttributeDescriptionCount = number of attribute descriptions
    -pVertexAttributeDescriptions = attributeDescriptions

    Current Vertex data includes:
    -position
    -color
    -texture coordinates
    // Note the definition for Vertex is defined by myself, not Vulkan, and is found in vulkan/vk_vertex.hpp

    Why:
    A vertex buffer is fundamentally a region of bytes.

    Vulkan needs to know:
    -how far apart vertices are in memory
    -which attributes exist
    -their formats
    -their offsets
    -which shader input locations receive them

    Important Consequences:
    -The vertex shader receives the correct position/color/UV data from the bound vertex buffer.
    -The C++ Vertex layout and shader input layout must remain compatible.

    Source Mapping:
    Original lines 489-494.


### Block 26 — Input Assembly State

    Purpose:
    Define how incoming vertices/indices are grouped into geometric primitives.

    My Configuration:
    -topology = TriangleList
    -primitiveRestartEnable = false

    Why:
    TriangleList interprets every group of three referenced vertices as an independent triangle.

    Primitive restart is disabled because the current indexed triangle-list rendering does not use special restart indices to split primitive sequences.

    Important Consequences:
    -The index buffer is interpreted as triangle geometry.
    -Every three indices form a triangle.
    -No primitive-restart sentinel values are currently recognized.

    Source Mapping:
    Original lines 496-501.


### Block 27 — Viewport and Scissor State

    Purpose:
    Define how rendered coordinates map into the framebuffer and which framebuffer region is allowed to receive fragments.

    My Configuration:
    PipelineViewportStateCreateInfo:
    -viewportCount = 1
    -scissorCount = 1

    Both viewport and scissor are configured as dynamic states and supplied while recording command buffers.

    Why:
    Viewport:
    -Maps normalized device coordinates into framebuffer coordinates/depth range.

    Scissor:
    -Defines a rectangular region of the framebuffer.
    -Fragments outside the rectangle are discarded.

    viewportCount = 1 does not mean "one camera." It means the pipeline currently uses one viewport.

    Important Consequences:
    -The renderer currently uses one viewport and one matching scissor rectangle.
    -Their dimensions can change without recreating the entire graphics pipeline because they are dynamic.
    -Window/swapchain resizing can update these values during command recording.

    Source Mapping:
    Original lines 503-506 and dynamic-state configuration.


### Block 28 — Rasterization State

    Purpose:
    Configure how assembled primitives are converted into rasterized fragments and which triangle faces are allowed to survive rasterization.

    My Configuration:
    -depthClampEnable = false
    -rasterizerDiscardEnable = false
    -polygonMode = Fill
    -cullMode = Back
    -frontFace = CounterClockwise
    -depthBiasEnable = false
    -lineWidth = 1.0f

    Why:
    depthClampEnable = false:
    -Primitives outside the normal depth range are handled through normal clipping rather than depth clamping.

    rasterizerDiscardEnable = false:
    -Rasterization remains enabled.
    -Setting this true prevents primitives from proceeding through rasterization.
    -This does NOT convert the graphics pipeline into a compute pipeline.

    polygonMode = Fill:
    -Triangle interiors are rasterized normally.

    cullMode = Back:
    -Triangles classified as back-facing are discarded before fragment processing.

    frontFace = CounterClockwise:
    -Defines counterclockwise projected winding as the front-facing orientation.
    -Winding is primarily used for front/back-face classification and culling.

    depthBiasEnable = false:
    -No additional depth bias is currently applied.

    lineWidth = 1.0f:
    -Uses normal line width for relevant line rendering.

    Important Consequences:
    -Only front-facing filled triangles survive back-face culling.
    -Incorrect model winding relative to our chosen frontFace can cause intended surfaces to be culled.
    -The rasterization stage remains active.
    -No depth-bias technique is currently used for issues such as shadow-map/self-overlap artifacts.

    Source Mapping:
    Original lines 509-524.


### Block 29 — Multisample State

    Purpose:
    Configure the graphics pipeline's multisample behavior for MSAA.

    My Configuration:
    -rasterizationSamples = msaaSamples
    -sampleShadingEnable = false

    Why:
    rasterizationSamples must match the sample count of the multisampled color/depth attachments.

    MSAA stores multiple coverage/sample locations per pixel so triangle edges can be resolved more smoothly.

    sampleShadingEnable = false means the renderer is not explicitly enabling sample-rate shading for a configured minimum fraction of samples.

    MSAA still operates with multiple samples; disabling sample shading does not simply mean "the fragment shader always runs exactly once per pixel" in every possible implementation circumstance.

    Important Consequences:
    -The graphics pipeline is compatible with our multisampled color/depth attachments.
    -Geometry edges can receive MSAA antialiasing.
    -Per-sample fragment shading is not deliberately requested.
    -The multisampled color result later resolves into the single-sampled swapchain image.

    Source Mapping:
    Original lines 526-531.


### Block 30 — Depth and Stencil State

    Purpose:
    Configure depth testing/writing and stencil behavior for the graphics pipeline.

    My Configuration:
    -depthTestEnable = true
    -depthWriteEnable = true
    -depthCompareOp = Less
    -depthBoundsTestEnable = false
    -stencilTestEnable = false

    Why:
    Depth testing compares an incoming fragment's depth value against the value already stored in the depth attachment.

    CompareOp::Less means the incoming fragment passes when its depth value is less than the stored value.

    depthWriteEnable allows passing fragments to update the depth attachment.

    Depth-bounds and stencil testing are not currently needed by WizEngine.

    Important Consequences:
    -Closer opaque surfaces can occlude farther surfaces.
    -Passing fragments update the depth buffer.
    -Stencil-based effects such as certain outline/masking techniques are not currently available through this pipeline configuration.
    -Depth values here are pipeline depth-buffer values, not raw world-space Z coordinates.

    Source Mapping:
    Original lines 533-541.


### Block 31 — Color Blending State

    Purpose:
    Configure how fragment-shader color output interacts with the existing color attachment.

    My Configuration:

    ColorBlend attachment:
    -blendEnable = false
    -colorWriteMask:
        -R
        -G
        -B
        -A

    Pipeline-level:
    -logicOpEnable = false
    -logicOp = Copy
        -Copy is specified, but has no effect while logic operations are disabled.
    -attachmentCount = 1
    -pAttachments = colorBlendAttachment

    Why:
    With blending disabled, the fragment's output color replaces the destination color for channels enabled by the write mask.

    This does NOT require the shader's alpha value to be 1.0.

    Alpha values can still be written to the attachment; they simply are not currently used to blend the source color with the existing destination color.

    Logic operations are currently disabled, so the configured Copy operation is inactive.

    Important Consequences:
    -Opaque rendering behaves as expected.
    -All RGBA channels can be written.
    -Normal transparency blending is not currently implemented.
    -Supporting transparent materials later will require appropriate blending state and usually additional rendering-order/material logic.

    Source Mapping:
    Original lines 543-553.


### Block 32 — Dynamic Pipeline State

    Purpose:
    Mark selected graphics-pipeline state as values that can be supplied or changed during command recording without rebuilding the pipeline.

    My Configuration:
    Dynamic states:
    -Viewport
    -Scissor

    Fill PipelineDynamicStateCreateInfo with:
    -dynamicStateCount = dynamicStates.size()
    -pDynamicStates = dynamicStates.data()

    Why:
    Window/swapchain dimensions can change.

    If viewport and scissor were static pipeline state, changing their dimensions could require creating a new pipeline.

    Making them dynamic allows command buffers to provide appropriate values for the current render extent.

    Important Consequences:
    -Viewport/scissor can change between command-buffer recordings.
    -Window resizing is easier to support.
    -Other non-dynamic pipeline state still remains baked into the pipeline.

    Source Mapping:
    Original lines 555-562.


### Block 33 — Dynamic Rendering Pipeline Compatibility

    Purpose:
    Tell the graphics pipeline which attachment formats it will be used with when using Vulkan dynamic rendering.

    My Configuration:
    PipelineRenderingCreateInfo:
    -colorAttachmentCount = 1
    -pColorAttachmentFormats = colorFormat
    -depthAttachmentFormat = depthFormat

    GraphicsPipelineCreateInfo:
    -renderPass = nullptr

    Attach PipelineRenderingCreateInfo to GraphicsPipelineCreateInfo through Vulkan's pNext chain using vk::StructureChain.

    Why:
    Traditional Vulkan render-pass-based pipelines obtain attachment compatibility information from a RenderPass.

    WizEngine uses dynamic rendering instead.

    Therefore:
    -GraphicsPipelineCreateInfo does not reference a traditional RenderPass.
    -PipelineRenderingCreateInfo supplies the color/depth attachment-format information required for pipeline compatibility.

    vk::StructureChain automatically connects compatible structures through pNext.

    Passing:
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>()

    passes the root GraphicsPipelineCreateInfo, whose pNext points to the PipelineRenderingCreateInfo also stored in the chain.

    Important Consequences:
    -The pipeline can be used with dynamic rendering without creating a traditional VkRenderPass.
    -renderPass is nullptr because attachment compatibility is supplied through PipelineRenderingCreateInfo instead.
    -Rendering attachments must use compatible formats.
    -The pipeline knows about our color and depth attachment formats at creation time.

    Source Mapping:
    Original lines 567-575.


### Block 34 — Final Graphics Pipeline Creation

    Purpose:
    Combine all previously configured graphics-pipeline state into the final Vulkan graphics pipeline object.

    My Configuration:
    GraphicsPipelineCreateInfo:
    -stageCount = 2
    -pStages = shader stages (vertex + fragment)
    -pVertexInputState = vertexInputInfo
    -pInputAssemblyState = inputAssembly
    -pViewportState = viewportState
    -pRasterizationState = rasterizer
    -pMultisampleState = multisampling
    -pDepthStencilState = depthStencil
    -pColorBlendState = colorBlending
    -pDynamicState = dynamicState
    -layout = pipelineLayout
    -renderPass = nullptr
    -dynamic rendering information attached through pNext

    Create vk::raii::Pipeline using:
    -device
    -pipeline cache = nullptr
    -root GraphicsPipelineCreateInfo from pipelineCreateInfoChain

    Return the completed Pipeline from makeGraphicsPipeline().

    createGraphicsPipeline() stores the returned object in:
    -graphicsPipeline

    Why:
    Every previous pipeline section defines one part of the complete graphics processing configuration.

    Final pipeline creation packages those pieces into the Vulkan Pipeline object that will later be bound in a command buffer before drawing.

    The pipeline cache argument is currently nullptr, meaning WizEngine is not currently supplying a PipelineCache during pipeline creation.

    Important Consequences:
    -graphicsPipeline is ready to be bound during rendering.
    -The pipeline contains two programmable shader stages: vertex and fragment.
    -The pipeline has a defined shader/resource/fixed-function configuration.
    -Dynamic rendering compatibility is supplied through the pNext chain instead of a traditional RenderPass.
    -No PipelineCache is currently used.
    -Most non-dynamic state changes require creation of another pipeline.

    Source Mapping:
    Original lines 446-460 and 573-577.


# Command Infrastructure

### Block 35 — Command Pool / `createCommandPool()`

    Purpose:
    Create the allocation/lifetime context from which graphics command buffers will be allocated.

    My Configuration:
    CommandPoolCreateInfo:
    -flags = ResetCommandBuffer
    -queueFamilyIndex = graphicsIndex

    Why:
    A command pool is a driver-managed allocation/bookkeeping context for command buffers.

    It is associated with one queue family because command buffers allocated from it are intended for queues belonging to that family.

    ResetCommandBuffer allows command buffers allocated from the pool to be individually reset and re-recorded.

    The command pool is not a lookup structure used to select command buffers during rendering. The application stores and uses the command-buffer handles directly.

    However, the pool does have a lifetime relationship with its command buffers: destroying the pool frees command buffers allocated from it.

    Important Consequences:
    -Command buffers can now be allocated for the graphics queue family.
    -Individual command buffers can be reset and re-recorded.
    -Temporary upload command buffers can also be allocated from this pool.
    -Destroying the command pool invalidates/frees command buffers allocated from it.

    Source Mapping:
    Original lines 581-600.


### Block 36 — Single-Use Command Helpers / `beginSingleTimeCommands()` + `endSingleTimeCommands()`

    Purpose:
    Provide a simple helper pattern for allocating, recording, submitting, and completing short-lived GPU commands used during resource setup and uploads.

    My Configuration:

    beginSingleTimeCommands():

    Fill CommandBufferAllocateInfo:
    -commandPool = commandPool
    -level = Primary
    -commandBufferCount = 1

    Allocate:
    -One primary vk::raii::CommandBuffer from commandPool.
    -Move the first returned command buffer into the local commandBuffer variable.

    Fill CommandBufferBeginInfo:
    -flags = OneTimeSubmit

    Begin recording:
    -commandBuffer.begin(beginInfo)

    Return:
    -commandBuffer

    OneTimeSubmit indicates that the recorded command buffer is intended for a one-time submission before being reset/re-recorded or released.
    It is a usage hint; it does not automatically submit or destroy the command buffer.

    endSingleTimeCommands():

    End recording:
    -commandBuffer.end()

    Fill SubmitInfo:
    -commandBufferCount = 1
    -pCommandBuffers = commandBuffer

    Submit:
    -graphicsQueue.submit(submitInfo, nullptr)
    -No fence is supplied for this submission.
    -No wait or signal semaphores are used by this SubmitInfo.

    Wait for completion:
    -graphicsQueue.waitIdle()

    waitIdle() waits until all previously submitted work on graphicsQueue has completed.

    This guarantees that the single-use command buffer has finished executing before the helper returns, although it is broader than waiting only for this specific submission.

    The command buffer is stored through vk::raii::CommandBuffer.
    It is not manually freed inside endSingleTimeCommands(); its underlying Vulkan command buffer is released through RAII when its owning wrapper later leaves scope.

    The rvalue-reference parameter:
        vk::raii::CommandBuffer&&
    communicates that the caller is finished using this temporary command buffer after submission.

    Why:
    Resource creation frequently requires one-off GPU operations such as:
    -buffer copies
    -buffer-to-image copies
    -image layout transitions
    -mipmap generation
    These operations do not need to remain in the normal per-frame command buffers.

    The helper provides a simple sequence:
        allocate primary command buffer
            ↓
        begin OneTimeSubmit recording
            ↓
        record setup/upload commands
            ↓
        end recording
            ↓
        submit to graphicsQueue
            ↓
        wait for graphicsQueue to become idle
            ↓
        temporary command buffer can later be released

    Important Consequences:
    -Upload/setup commands remain separate from normal per-frame rendering commands.
    -The temporary command buffer comes from the same commandPool used for the graphics queue family.
    -The command buffer is Primary and can therefore be submitted directly to graphicsQueue.
    -The helper does not require semaphores or a fence because it immediately waits for the entire graphicsQueue to become idle.
    -graphicsQueue.waitIdle() makes the helper simple and safe for initialization-time work, but also fully synchronizes the graphics queue.
    -A future asynchronous upload system could replace this coarse wait with fences, dedicated transfer queues, or more advanced synchronization.
    -The command buffer itself is managed through Vulkan-Hpp RAII rather than manually freed in this helper.

    Source Mapping:
    The original texture documentation referenced beginning and ending a single-time command buffer around upload work at lines 618-622, but did not separately document these helper functions.


### Block 37 — Buffer Copy Helper / `copyBuffer()`

    Purpose:
    Record and execute a GPU-side copy from one Vulkan Buffer into another.

    My Configuration:

    Function inputs:
    -srcBuffer = source Vulkan Buffer
    -dstBuffer = destination Vulkan Buffer
    -size = number of bytes to copy

    Begin:
    -Create a temporary command buffer using beginSingleTimeCommands().

    Record BufferCopy:
    -srcOffset = 0
    -dstOffset = 0
    -size = requested copy size

    Record:
    -commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, BufferCopy)

    This copies `size` bytes beginning at byte 0 of srcBuffer into byte 0 of dstBuffer.

    End:
    -Pass the temporary command buffer to endSingleTimeCommands().
    -The command buffer is ended and submitted to graphicsQueue.
    -With the current single-use helper implementation, graphicsQueue.waitIdle() ensures the copy has completed before copyBuffer() returns.

    Typical current uses:
    -staging buffer -> vertexBuffer
    -staging buffer -> indexBuffer

    Why:
    Device-local vertex/index buffers are not written directly through a CPU pointer in the current renderer.

    Instead:
        CPU writes staging buffer
            ↓
        copyBuffer()
            ↓
        GPU executes buffer-to-buffer transfer
            ↓
        final device-local buffer receives the data

    BufferCopy separates the copy operation from the buffers themselves by describing:
    -where reading begins in the source buffer
    -where writing begins in the destination buffer
    -how many bytes are transferred

    The current helper always begins both ranges at offset 0.

    Important Consequences:
    -Static vertex/index data can be uploaded from CPU-accessible staging memory into device-local buffers.
    -Exactly the requested `size` bytes are copied; this does not inherently mean the entire source or destination buffer is copied.
    -The current helper only supports copies beginning at offset 0.
    -The source and destination buffers must have been created for appropriate transfer usage by the caller.
    -The staging allocation can be released after the copy completes.
    -Because endSingleTimeCommands() currently waits for graphicsQueue to become idle, the transfer is complete before this helper returns.
    -copyBuffer() centralizes buffer-transfer command recording instead of duplicating it in each uploader.

    Source Mapping:
    The original vertex/index documentation referenced copyBuffer() at lines 724-725 and 742-743 but did not separately explain the helper.


# Texture System

### Texture Upload Mental Model

    Disk image (.png/.jpg)
        ↓ decode with stb_image
    CPU pixel data
        ↓
    host-visible staging buffer
        ↓ GPU transfer
    device-local texture image
        ↓
    generate/populate mip levels
        ↓
    ImageView
        +
    Sampler
        ↓
    Descriptor
        ↓
    Fragment Shader


### Block 38 — Texture Image Upload / `createTextureImage()`

    Purpose:
    Decode texture data from disk, upload the base pixels through a staging buffer, and create a device-local Vulkan texture image containing the full mip chain.

    My Configuration:

    Load texture using stbi_load():
    -path = TEXTURE_PATH
    -output width = texWidth
    -output height = texHeight
    -output channel information = texChannels
    -requested output format = STBI_rgb_alpha

    STBI_rgb_alpha forces the returned pixel data into 4-channel RGBA format regardless of the source image's original channel count.

    -If loading fails:
        -throw runtime_error

    Calculate mip levels:
    -mipLevels = floor(log2(max(texWidth, texHeight))) + 1

    This creates enough levels to reduce the largest image dimension down to 1 pixel.

    Calculate upload size:
    -imageSize = texWidth × texHeight × 4 bytes

    The ×4 matches the forced RGBA output from stb_image.

    Create staging buffer using createBuffer():
    -size = imageSize
    -usage = TransferSrc
    -memory = HostVisible | HostCoherent

    Upload CPU pixel data:
    -Map stagingBufferMemory from offset 0 for imageSize bytes.
    -memcpy the decoded pixel data into mapped staging memory.
    -Unmap stagingBufferMemory.
    -Free the original stb_image pixel allocation using stbi_image_free().

    Create final texture image using createImage():
    -width = texWidth
    -height = texHeight
    -mipLevels = calculated mipLevels
    -samples = e1
    -format = R8G8B8A8Srgb
    -tiling = Optimal
    -usage:
        -TransferSrc
        -TransferDst
        -Sampled
    -memory = DeviceLocal

    Store:
    -textureImage = std::move(image)
    -textureImageMemory = std::move(imageMemory)

    TransferDst:
    -Allows pixel data to be copied into the image.

    TransferSrc:
    -Allows mip levels to be used as transfer sources during mipmap generation.

    Sampled:
    -Allows the completed image to later be sampled by shaders.

    Begin one single-use command buffer.

    Record into that command buffer:

    1. transitionImageLayout():
        -image = textureImage
        -old layout = Undefined
        -new layout = TransferDstOptimal
        -mipLevels = full mip count

    2. copyBufferToImage():
        -source = stagingBuffer
        -destination = textureImage
        -width = texWidth
        -height = texHeight

        This uploads the decoded base texture data into the texture image.

    3. generateMipmaps():
        -image = textureImage
        -format = R8G8B8A8Srgb
        -width = texWidth
        -height = texHeight
        -mipLevels = full mip count

        Mipmap generation and the required per-mip layout transitions are handled by generateMipmaps().

    End and submit the same single-use command buffer using endSingleTimeCommands().

    With the current helper implementation, graphicsQueue.waitIdle() ensures these upload commands have completed before createTextureImage() returns.

    Why:
    The final texture should live in device-local GPU memory for efficient shader access.

    CPU-decoded pixel data is therefore uploaded using a staging path:
        image file
            ↓ stbi_load()
        CPU RGBA pixels
            ↓ memcpy
        host-visible staging buffer
            ↓ GPU transfer
        device-local textureImage
            ↓
        mipmap generation
            ↓
        shader-readable texture resource

    The texture image is created with storage for the entire mip chain up front.

    generateMipmaps() fills those already-created mip subresources rather than allocating additional Vulkan Images.

    Important Consequences:
    -The source texture is decoded into 4-channel RGBA pixel data.
    -textureImage uses the R8G8B8A8Srgb format.
    -textureImage is single-sampled because it is a sampled texture resource rather than the MSAA render target.
    -Mip level 0 receives the original decoded texture data.
    -Storage for the full mip chain exists when createImage() returns.
    -TransferSrc and TransferDst usages support the upload/mipmap-generation process.
    -Sampled usage allows the completed texture to later be accessed by shaders.
    -The staging buffer and its memory are temporary local RAII resources and can be released after the upload work completes.
    -textureImage and textureImageMemory remain as the persistent device-local texture resources.

    Source Mapping:
    Original lines 602-633.


### Block 39 — Image Layout Transition / `transitionImageLayout()`

    Purpose:
    Record synchronization barriers that transition Vulkan images between layouts appropriate for their next use.

    WizEngine currently provides two transitionImageLayout() overloads.

    Generic transitionImageLayout():

    Inputs:
    -commandBuffer
    -image
    -oldLayout
    -newLayout
    -srcAccessMask
    -dstAccessMask
    -srcStageMask
    -dstStageMask
    -aspectFlags

    Fill ImageMemoryBarrier2:

    Source synchronization:
    -srcStageMask = supplied source stage
    -srcAccessMask = supplied source access

    Destination synchronization:
    -dstStageMask = supplied destination stage
    -dstAccessMask = supplied destination access

    Layout:
    -oldLayout = supplied old layout
    -newLayout = supplied new layout

    Queue-family ownership:
    -srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
    -dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED

    Image/subresource:
    -image = supplied image
    -aspectMask = supplied aspectFlags
    -baseMipLevel = 0
    -levelCount = 1
    -baseArrayLayer = 0
    -layerCount = 1

    This overload is flexible because the caller explicitly defines the synchronization and image aspect.
    It currently transitions mip level 0 only.

    Texture-oriented transitionImageLayout():

    Inputs:
    -commandBuffer
    -image
    -oldLayout
    -newLayout
    -mipLevels

    Supported transition #1:
        Undefined
            ↓
        TransferDstOptimal

    Synchronization:
    -sourceStage = TopOfPipe
    -sourceAccess = none
    -destinationStage = Transfer
    -destinationAccess = TransferWrite

    This prepares newly-created image subresources to receive transfer writes.

    Supported transition #2:
        TransferDstOptimal
            ↓
        ShaderReadOnlyOptimal

    Synchronization:
    -sourceStage = Transfer
    -sourceAccess = TransferWrite
    -destinationStage = FragmentShader
    -destinationAccess = ShaderRead

    This prepares transferred image data for fragment-shader sampling.

    Any other oldLayout/newLayout combination:
    -throw runtime_error

    Texture ImageMemoryBarrier2:

    Queue-family ownership:
    -srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
    -dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED

    Subresource range:
    -aspectMask = Color
    -baseMipLevel = 0
    -levelCount = mipLevels
    -baseArrayLayer = 0
    -layerCount = 1

    Unlike the generic overload, this version can transition the full supplied mip range at once.

    Both overloads:

    Fill DependencyInfo:
    -imageMemoryBarrierCount = 1
    -pImageMemoryBarriers = barrier

    Record:
    -commandBuffer.pipelineBarrier2(dependencyInfo)

    pipelineBarrier2() records the barrier into the command buffer.
    The transition executes later when that command buffer is submitted.

    Why:
    Vulkan images must be in layouts appropriate for how they are being used.

    ImageMemoryBarrier2 describes both:
    -the synchronization between previous and future image accesses
    -the image layout transition itself

    Conceptually:
        previous use
            ↓
        source stage/access
            ↓
        ImageMemoryBarrier2
            ↓
        layout transition
            ↓
        destination stage/access
            ↓
        next use

    The generic overload allows the caller to specify all synchronization details.

    The texture-oriented overload packages the synchronization rules currently needed by the texture-upload path.

    Important Consequences:
    -WizEngine uses Synchronization2 through ImageMemoryBarrier2 and pipelineBarrier2().
    -Neither overload performs queue-family ownership transfers.
    -The generic overload supports caller-selected image aspects but currently transitions one mip level.
    -The texture-oriented overload always targets the Color aspect and can transition multiple mip levels.
    -The texture-oriented overload currently supports only:
        -Undefined -> TransferDstOptimal
        -TransferDstOptimal -> ShaderReadOnlyOptimal
    -Unsupported texture-layout transitions cause an exception.
    -The initial createTextureImage() transition from Undefined -> TransferDstOptimal uses the texture-oriented overload and applies to the full mip range.

    Source Mapping:
    transitionImageLayout() overloads used by image synchronization and texture upload operations.


### Block 40 — Buffer-to-Image Copy / `copyBufferToImage()`

    Purpose:
    Record a command that copies pixel data from a Vulkan Buffer into the base mip level of a Vulkan Image.

    My Configuration:

    Function inputs:
    -commandBuffer = command buffer receiving the copy command
    -buffer = source Vulkan Buffer
    -image = destination Vulkan Image
    -width = width of the copied image region
    -height = height of the copied image region

    Fill BufferImageCopy region:

    Buffer layout:
    -bufferOffset = 0
        -Begin reading from byte 0 of the source buffer.
    -bufferRowLength = 0
    -bufferImageHeight = 0
        -Zero tells Vulkan to treat the corresponding buffer dimensions as tightly packed according to imageExtent.
        -No custom row length or image-height padding is specified.

    Destination image subresource:
    -aspectMask = Color
    -mipLevel = 0
    -baseArrayLayer = 0
    -layerCount = 1

    This means the copy targets:
    -the image's color aspect
    -base mip level 0
    -array layer 0
    -one array layer

    Destination offset:
    -imageOffset = {0, 0, 0}
    The copy begins at the origin of the destination image.

    Copy extent:
    -imageExtent = {width, height, 1}

    The copied region therefore covers:
    -the supplied width
    -the supplied height
    -a depth of 1 texel
    This matches the current 2D texture-upload path.

    Record copy command:
        commandBuffer.copyBufferToImage(buffer, image, TransferDstOptimal, region)

    The destination image is expected to be in:
    -TransferDstOptimal
    when the copy executes.

    Why:
    createTextureImage() initially places decoded texture pixels into a host-visible staging buffer.

    copyBufferToImage() records the GPU operation that transfers those pixels into the device-local texture image:
        stagingBuffer
            ↓
        BufferImageCopy region
            ↓
        textureImage mip 0

    BufferImageCopy describes both:
    -where the source data begins in the buffer
    -which part of the destination image receives that data

    The helper does not perform image-layout transitions itself.

    In the current texture-upload sequence:
        transitionImageLayout()
            ↓
        textureImage becomes TransferDstOptimal
            ↓
        copyBufferToImage()
            ↓
        base pixels copied into mip level 0
            ↓
        generateMipmaps()

    Important Consequences:
    -The current helper copies from byte offset 0 of the source buffer.
    -The source pixel data is interpreted as tightly packed.
    -The copy writes only to mip level 0.
    -The copy writes only to the Color aspect.
    -The copy targets array layer 0 with layerCount = 1.
    -The destination region begins at image coordinate {0, 0, 0}.
    -The copy extent is width × height × 1.
    -The destination image must already be in TransferDstOptimal when the command executes.
    -The helper records the command into an existing commandBuffer; it does not begin, end, submit, or wait on that command buffer.
    -No synchronization or layout barrier is created by this helper itself.
    -Later mip levels are not populated here; they are handled by generateMipmaps().

    Source Mapping:
    copyBufferToImage() helper used by createTextureImage() during texture upload.


### Block 41 — Runtime Mipmap Generation / `generateMipmaps()`

    Purpose:
    Generate progressively smaller mip levels from the uploaded base texture and transition every completed mip level into ShaderReadOnlyOptimal for later texture sampling.

    My Configuration:

    Function inputs:
    -commandBuffer = command buffer receiving the mip-generation commands
    -image = texture image containing the mip chain
    -imageFormat = texture image format
    -texWidth = base texture width
    -texHeight = base texture height
    -mipLevels = total number of mip levels

    Check format support:

    Query:
    -formatProperties = physicalDevice.getFormatProperties(imageFormat)

    Require:
    -formatProperties.optimalTilingFeatures contains SampledImageFilterLinear

    If unsupported:
    -throw runtime_error

    This verifies that the texture format supports the linear filtering required by the current linear blit-based mipmap generation.

    Initialize working dimensions:
    -mipWidth = texWidth
    -mipHeight = texHeight

    For each mip level:
        i = 1 -> mipLevels - 1
    The previous mip:
        i - 1
    becomes the source used to generate:
        mip i

    Step 1 — Prepare the previous mip as a transfer source:

    Create:
    -vk::ImageMemoryBarrier2 toTransferSrc

    Synchronization:
    -srcStageMask = Transfer
    -srcAccessMask = TransferWrite
    -dstStageMask = Transfer
    -dstAccessMask = TransferRead

    Layout transition:
    -oldLayout = TransferDstOptimal
    -newLayout = TransferSrcOptimal

    Queue-family ownership:
    -srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
    -dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED

    Target image:
    -image = image

    Subresource range:
    -aspectMask = Color
    -baseMipLevel = i - 1
    -levelCount = 1
    -baseArrayLayer = 0
    -layerCount = 1

    This barrier targets exactly the previous mip level.

    It changes that mip from:
        transfer destination
            ->
        transfer source
    so it can be read by the upcoming blit.

    Create:
    -vk::DependencyInfo toTransferSrcDependency

    DependencyInfo:
    -imageMemoryBarrierCount = 1
    -pImageMemoryBarriers = &toTransferSrc

    This DependencyInfo contains the single ImageMemoryBarrier2 that performs the transition/synchronization above.

    Record:
    -commandBuffer.pipelineBarrier2(toTransferSrcDependency)

    pipelineBarrier2() records that dependency into the command buffer.

    Why:
    The previous mip was either:
    -mip 0, populated by the original buffer-to-image upload
    or:
    -a mip produced by the previous loop iteration

    In either case, it is currently in TransferDstOptimal.

    Before Vulkan can read from it as the source of a blit, it must be transitioned to TransferSrcOptimal with transfer-read access.

    Step 2 — Describe the blit from the previous mip into the next mip:

    Create:
    -vk::ImageBlit blit

    Source subresource:

    blit.srcSubresource:
    -aspectMask = Color
    -mipLevel = i - 1
    -baseArrayLayer = 0
    -layerCount = 1

    Source offsets:
    -srcOffsets[0] = {0, 0, 0}
    -srcOffsets[1] = {mipWidth, mipHeight, 1}

    Together these offsets describe the full current source mip region.

    Destination subresource:

    blit.dstSubresource:
    -aspectMask = Color
    -mipLevel = i
    -baseArrayLayer = 0
    -layerCount = 1

    Destination offsets:
    -dstOffsets[0] = {0, 0, 0}

    -dstOffsets[1] = {
        mipWidth > 1 ? mipWidth / 2 : 1,
        mipHeight > 1 ? mipHeight / 2 : 1,
        1
    }

    This makes the destination mip approximately half the width and height of the source mip while preventing either dimension from becoming 0.

    Record the blit:

    -commandBuffer.blitImage(image, TransferSrcOptimal, image, TransferDstOptimal, blit, Linear)

    Source:
    -same Vulkan image
    -mip i - 1
    -layout = TransferSrcOptimal

    Destination:
    -same Vulkan image
    -mip i
    -layout = TransferDstOptimal

    Filter:
    -Linear

    The same Vulkan Image is used as both source and destination, but different mip subresources are selected by ImageBlit.

    The linear blit downsamples mip i - 1 into mip i.

    Step 3 — Make the completed source mip shader-readable:

    After the blit, mip i - 1 has finished being used as a transfer source.

    Create:
    -vk::ImageMemoryBarrier2 toShaderRead

    Synchronization:
    -srcStageMask = Transfer
    -srcAccessMask = TransferRead
    -dstStageMask = FragmentShader
    -dstAccessMask = ShaderRead

    Layout transition:
    -oldLayout = TransferSrcOptimal
    -newLayout = ShaderReadOnlyOptimal

    Queue-family ownership:
    -srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
    -dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED

    Target image:
    -image = image

    Subresource range:
    -aspectMask = Color
    -baseMipLevel = i - 1
    -levelCount = 1
    -baseArrayLayer = 0
    -layerCount = 1

    This targets the same previous mip that was just used as the blit source.

    Create:
    -vk::DependencyInfo toShaderReadDependency

    DependencyInfo:
    -imageMemoryBarrierCount = 1
    -pImageMemoryBarriers = &toShaderRead

    This DependencyInfo contains the barrier that transitions the completed source mip into its final shader-readable state.

    Record:
    -commandBuffer.pipelineBarrier2(toShaderReadDependency)

    The previous mip is now finished with mipmap-generation transfer operations and is prepared for later fragment-shader sampling.

    Step 4 — Update the working mip dimensions:

    If mipWidth > 1:
    -mipWidth /= 2

    If mipHeight > 1:
    -mipHeight /= 2

    These values now describe the dimensions of the mip that was just generated.

    Neither dimension is allowed to shrink below 1.


    Loop behavior:

    Each iteration therefore performs:
        mip i - 1 = TransferDstOptimal
                ↓
        toTransferSrc barrier
                ↓
        mip i - 1 = TransferSrcOptimal
                ↓
        blit into mip i
                ↓
        mip i remains TransferDstOptimal
                ↓
        toShaderRead barrier
                ↓
        mip i - 1 = ShaderReadOnlyOptimal

    On the next iteration, mip i becomes the new source and repeats the same process.

    After the loop:

    The final mip level:

        mipLevels - 1

    was written as a transfer destination during the final blit.

    Unlike every earlier mip, it is never needed as the source of another blit.

    It therefore remains:
    -TransferDstOptimal

    and requires one final transition.

    Create:
    -vk::ImageMemoryBarrier2 lastMipBarrier

    Synchronization:
    -srcStageMask = Transfer
    -srcAccessMask = TransferWrite
    -dstStageMask = FragmentShader
    -dstAccessMask = ShaderRead

    Layout transition:
    -oldLayout = TransferDstOptimal
    -newLayout = ShaderReadOnlyOptimal

    Queue-family ownership:
    -srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
    -dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED

    Target image:
    -image = image

    Subresource range:
    -aspectMask = Color
    -baseMipLevel = mipLevels - 1
    -levelCount = 1
    -baseArrayLayer = 0
    -layerCount = 1


    Create:
    -vk::DependencyInfo lastMipDependency

    DependencyInfo:
    -imageMemoryBarrierCount = 1
    -pImageMemoryBarriers = &lastMipBarrier

    Record:
    -commandBuffer.pipelineBarrier2(lastMipDependency)

    The final mip is now also in ShaderReadOnlyOptimal.

    Why:
    Before generateMipmaps() begins, the texture's mip levels have already been created and transitioned into TransferDstOptimal.

    Mip level 0 contains the original uploaded texture data.

    The function builds the mip chain progressively:
        mip 0
        ↓ linear blit
        mip 1
        ↓ linear blit
        mip 2
        ↓
        ...
        ↓
        final mip

    A mip must temporarily become TransferSrcOptimal when it is used to generate the next level.

    Once that mip has generated its successor, it no longer needs transfer access and is transitioned into ShaderReadOnlyOptimal.

    The destination mip remains TransferDstOptimal until the next iteration promotes it to the new transfer source.

    The final mip requires its own barrier because it never becomes the source of another blit.

    Barrier / Dependency relationship:
        ImageMemoryBarrier2
            ↓ referenced by
        DependencyInfo
            ↓ passed to
        commandBuffer.pipelineBarrier2()
            ↓
        synchronization + layout-transition command recorded

    Each of the three barrier cases in this function follows that structure:

    1. toTransferSrc
        -> toTransferSrcDependency
        -> pipelineBarrier2()

    2. toShaderRead
        -> toShaderReadDependency
        -> pipelineBarrier2()

    3. lastMipBarrier
        -> lastMipDependency
        -> pipelineBarrier2()

    Important Consequences:
    -Mipmaps are generated from the previous mip rather than repeatedly from the original full-resolution image.
    -Each successive mip is approximately half the previous width and height, with dimensions clamped to a minimum of 1.
    -Linear filtering is used during mip generation.
    -The texture format must support SampledImageFilterLinear for the current blit-based approach.
    -The same Vulkan Image is used as both blit source and destination, while different mip subresources are selected.
    -Each non-final mip progresses through:
        TransferDstOptimal
            ->
        TransferSrcOptimal
            ->
        ShaderReadOnlyOptimal
    -The final mip progresses directly from:
        TransferDstOptimal
            ->
        ShaderReadOnlyOptimal
    -Every synchronization barrier is placed inside its own DependencyInfo before being recorded with pipelineBarrier2().
    -Only the Color aspect is processed.
    -Only array layer 0 is processed.
    -Each barrier affects exactly one mip level.
    -No queue-family ownership transfers are performed.
    -The final result leaves the entire mip chain in ShaderReadOnlyOptimal.
    -generateMipmaps() only records commands into the supplied commandBuffer; submission happens elsewhere.
    -If mipLevels = 1, the loop is skipped and lastMipBarrier transitions mip 0 directly from TransferDstOptimal to ShaderReadOnlyOptimal.

    Source Mapping:
    generateMipmaps() helper used during texture-image creation after mip level 0 has been uploaded.


### Block 42 — Texture Image View / `createTextureImageView()`

    Purpose:
    Create the ImageView that exposes the texture image and its mip chain for shader resource access.

    My Configuration:
    createImageView():
    -image = textureImage
    -viewType = 2D
    -format = R8G8B8A8Srgb
    -aspectMask = Color
    -baseMipLevel = 0
    -levelCount = mipLevels
    -baseArrayLayer = 0
    -layerCount = 1

    Why:
    The ImageView defines how Vulkan should interpret and expose the texture image.

    In this case, the view exposes:
    -a 2D color image
    -the full mip chain
    -one array layer

    Important Consequences:
    -Shaders/descriptors can reference textureImage through textureImageView.
    -The sampler can select among mip levels visible through this view.
    -textureImageView does not own or duplicate the texture pixels; it references the existing image.

    Source Mapping:
    Original lines 635-653.
   


### Block 43 — Texture Sampler / `createTextureSampler()`

    Purpose:
    Create the Vulkan Sampler that defines how shaders sample texels and mip levels from the texture image.

    My Configuration:

    Query physical-device properties:
    -properties = physicalDevice.getProperties()

    Current use:
    -properties.limits.maxSamplerAnisotropy

    This allows the sampler's anisotropy setting to use the maximum anisotropy supported by the selected physical device.

    Fill SamplerCreateInfo:

    Texel filtering:
    -magFilter = Linear
    -minFilter = Linear

    magFilter:
    -Controls texel filtering when the sampled texture is being magnified.
    minFilter:
    -Controls texel filtering when the sampled texture is being minified.
    Linear filtering blends neighboring texel samples rather than selecting only the nearest texel.

    Mipmap filtering:
    -mipmapMode = Linear
    Linear mipmap mode allows sampling to interpolate between neighboring mip levels based on the calculated Level Of Detail.

    This is separate from minFilter/magFilter:
        minFilter / magFilter
            = filtering between texels within a mip level
        mipmapMode
            = filtering/interpolation between mip levels

    Addressing:
    -addressModeU = Repeat
    -addressModeV = Repeat
    -addressModeW = Repeat

    Repeat causes normalized texture coordinates outside the usual 0.0-1.0 range to wrap.

    Example:
        U = 1.2 -> wraps approximately like U = 0.2
    All three sampler address dimensions are configured as Repeat.

    LOD configuration:
    -mipLodBias = 0.0f
    -minLod = 0.0f
    -maxLod = static_cast<float>(mipLevels)

    mipLodBias = 0.0:
    -No additional sampler LOD bias is applied.

    minLod / maxLod:
    -Define the floating-point range to which the sampler's calculated LOD is clamped.

    Important:
    -These values are LOD clamps, not simply mip-array indices.

    If the texture contains N mip levels:
        valid mip indices:
        0 -> N - 1
    Setting:
        maxLod = N
    does NOT create or imply the existence of mip index N.

    The texture image/ImageView still expose only the mip levels actually created for the resource.

    Anisotropic filtering:
    -anisotropyEnable = true
    -maxAnisotropy = properties.limits.maxSamplerAnisotropy

    Anisotropic filtering improves texture sampling quality when a textured surface is viewed at an oblique angle.

    The current sampler requests the selected GPU's supported maximum anisotropy.

    This relies on samplerAnisotropy having been enabled when the logical device was created.

    Comparison sampling:
    -compareEnable = false
    -compareOp = Always

    Comparison sampling is disabled.

    Therefore:
    -compareOp is configured as Always
    -but compareOp is not active under the current sampler configuration

    Comparison sampling is commonly useful for specialized sampling operations such as depth comparisons, but the current color texture sampler does not use it.

    Border color:
    -borderColor = IntOpaqueBlack

    This field would define the returned border value when using an addressing mode such as ClampToBorder.

    Current addressing uses:
    -Repeat
    Therefore borderColor is currently configured but not used by the normal texture sampling path.


    Texture coordinate mode:
    -unnormalizedCoordinates = false
    This enables normal normalized texture coordinates.

    Typical UV coordinates therefore use the normalized texture coordinate space around:
        (0.0, 0.0)
            ->
        (1.0, 1.0)
    Coordinates can still go outside this range, where the Repeat address mode determines their wrapping behavior.

    Create sampler:
    -textureSampler = vk::raii::Sampler(device, samplerInfo)
    Store the resulting RAII sampler in:
    -textureSampler

    Why:
    The Image, ImageView, and Sampler serve different roles:
        Image = contains the texture image resource/data

        ImageView = defines which image subresources are exposed and how they are interpreted

        Sampler = defines how those exposed texels/mip levels are sampled

    The current sampler provides:
        textureImage
            ↓ exposed through
        textureImageView
            ↓ sampled according to
        textureSampler
            ↓
        shader texture access

    Linear texel filtering smooths sampling within mip levels.

    Linear mipmap filtering smooths transitions between neighboring mip levels.

    Repeat addressing allows tiled textures.

    Anisotropic filtering improves sampling quality at steep viewing angles.

    Important Consequences:
    -Texture magnification uses linear filtering.
    -Texture minification uses linear filtering.
    -Transitions between mip levels can also be linearly interpolated.
    -Textures repeat when normalized coordinates leave the ordinary 0.0-1.0 range.
    -No LOD bias is currently applied.
    -LOD sampling begins with a minimum clamp of 0.0.
    -maxLod = mipLevels is a floating-point LOD clamp and does not represent an additional mip level.
    -Anisotropic filtering is enabled at the physical device's reported maximum supported level.
    -Comparison sampling is disabled, so compareOp is currently inactive.
    -borderColor is currently inactive because Repeat addressing is used rather than ClampToBorder.
    -Normalized texture coordinates are used.
    -textureSampler is stored as a vk::raii::Sampler for later use by the texture descriptor.

    Source Mapping:
    createTextureSampler() configuration and sampler creation.


# Asset and Mesh Loading

### Block 44 — Model Loading Architecture / `loadModel()` + `loadObj()`

    Purpose:
    Load model-file geometry into WizEngine's common CPU-side MeshData representation and select which loaded mesh currently supplies the renderer's active vertices and indices.

    My Configuration:

    loadModel():

    Load glTF/GLB test model:
    -meshDataGltf = loadGltf(MODEL_PATH)

    Load OBJ model:
    -meshData = loadObj(MODEL_PATH)

    Current renderer geometry:
    -vertices = std::move(meshData.vertices)
    -indices = std::move(meshData.indices)

    Important current behavior:
    -loadGltf() is currently being developed.
    -Its returned MeshData is stored in local variable meshDataGltf.
    -meshDataGltf is not currently copied/moved into the renderer's active vertices or indices.
    -The active render geometry still comes from loadObj(MODEL_PATH).

    Therefore the current flow is:
        MODEL_PATH
            ↓
        loadGltf()
            ↓
        meshDataGltf
            ↓ (Under development)
        currently not consumed further

        MODEL_PATH
            ↓
        loadObj()
            ↓
        meshData
            ↓
        move vertices / indices
            ↓
        engine vertices / indices
    Eventually, depending on the file type .gltf, .glb, or .obj the corresponding loader will be called.

    loadObj():

    Create empty:
    -MeshData meshData

    Create tinyobjloader output structures:
    -tinyobj::attrib_t attrib
    -vector<tinyobj::shape_t> shapes
    -vector<tinyobj::material_t> materials

    Create parser message strings:
    -warn
    -err


    Parse OBJ:
    Call:
    -tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        &warn,
        &err,
        path.c_str()
    )

    If LoadObj() fails:
    -throw runtime_error(warn + err)

    On success, attrib/shapes/materials contain the data produced by tinyobjloader.

    Current use:
    -attrib = used
    -shapes = used
    -materials = populated but not otherwise processed by this function
    -warn = only included in the exception when LoadObj() returns false
    -err = only included in the exception when LoadObj() returns false

    Create deduplication map:

    -std::unordered_map<Vertex, uint32_t> uniqueVertices

    This maps a unique Vertex value to its index in:
    -meshData.vertices

    Iterate geometry:

    For every:
    -shape in shapes

    For every:
    -index in shape.mesh.indices

    Create:
    -Vertex vertex{}

    Position:

    Read three floats from attrib.vertices using:

        3 * index.vertex_index

    Populate:
    -vertex.pos.x =
        attrib.vertices[3 * index.vertex_index + 0]
    -vertex.pos.y =
        attrib.vertices[3 * index.vertex_index + 1]
    -vertex.pos.z =
        attrib.vertices[3 * index.vertex_index + 2]

    Texture coordinates:

    Read two floats from attrib.texcoords using:
        2 * index.texcoord_index

    Populate:
    -vertex.texCoord.x = attrib.texcoords[2 * index.texcoord_index + 0]

    -vertex.texCoord.y = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]

    The V coordinate is explicitly flipped:
        original V
            ↓
        1.0 - V
    This compensates for the texture-coordinate orientation difference expected by the current rendering path.


    Vertex color:
    -vertex.color = {1.0f, 1.0f, 1.0f}

    Every imported OBJ vertex therefore currently receives white vertex color.

    No material-derived vertex color is assigned here.

    Vertex deduplication:

    Attempt:
    -uniqueVertices.insert({vertex, static_cast<uint32_t>(meshData.vertices.size())})

    Structured result:
    -it = iterator to the existing or newly inserted map entry
    -inserted = whether this Vertex was new

    If inserted == true:

    -append vertex to meshData.vertices

    The map value assigned to a new Vertex is the index that vertex will occupy in meshData.vertices.

    For every processed OBJ index:

    -meshData.indices.push_back(it->second)

    This happens regardless of whether the Vertex was:
    -newly inserted
    or:
    -already present in uniqueVertices

    Therefore repeated identical Vertex values reuse an existing vertex-buffer index rather than duplicating the entire Vertex.
    Example:
        OBJ references:
        A B C A C D

        unique vertices:
        [A, B, C, D]

        indices:
        [0, 1, 2, 0, 2, 3]
    Return:
    -meshData

    Why:
    File formats such as OBJ and glTF organize geometry according to their own file-format structures.

    WizEngine converts format-specific data into a common CPU-side representation:
        model file
            ↓
        format-specific loader
            ↓
        MeshData
        ├── vertices
        └── indices

    This keeps later Vulkan buffer creation independent from the original asset format.

    loadObj() also deduplicates identical Vertex values so shared vertices can be referenced through indices rather than stored repeatedly.

    Important Consequences:
    -MeshData is CPU-side geometry data; this function does not upload geometry into Vulkan buffers.
    -The active engine vertices and indices currently come from the OBJ loader.
    -The glTF/GLB loader is currently invoked, but its returned meshDataGltf is not yet integrated into active rendering.
    -The OBJ loader combines geometry encountered across all shapes into one MeshData.
    -Shape boundaries are not preserved in the returned MeshData.
    -The materials vector is populated by tinyobjloader but is not currently used by loadObj().
    -Positions are imported directly from attrib.vertices.
    -Texture coordinates are imported from attrib.texcoords.
    -The texture V coordinate is flipped using 1.0f - V.
    -All imported vertices currently receive white vertex color.
    -Identical constructed Vertex values are deduplicated through uniqueVertices.
    -Every source index still produces one meshData.indices entry.
    -Repeated vertices therefore reuse existing indices instead of duplicating vertex data.
    -The current OBJ code assumes index.texcoord_index references valid texture-coordinate data.
    -A later asset/scene system will be needed to retain multiple independently loaded models rather than moving one MeshData directly into global renderer vertices/indices.

    // FUTURE ROBUSTNESS:
    // The current OBJ path directly indexes attrib.texcoords using
    // index.texcoord_index.
    //
    // If an OBJ contains vertices without texture coordinates,
    // texcoord_index may require validation before indexing.
    //
    // Materials are also loaded by tinyobjloader but are not yet used
    // by the current MeshData conversion.

    Source Mapping:
    loadModel() and loadObj() model-loading path.


# GPU Buffer Upload

### Block 45 — Staging Buffer Upload Pattern

    Purpose:
    Define the common CPU-to-GPU transfer pattern used for relatively static device-local buffer data.

    My Configuration / Mental Model:
    // Note: Host = CPU, Device = GPU
        CPU data
            ↓
    host-visible staging buffer
            ↓ copyBuffer()
    device-local destination buffer
            ↓
    GPU rendering use

    Why:
    Host-visible memory is convenient for CPU writes.

    In this renderer, device-local memory is preferred for frequently-read static GPU resources such as vertex/index data.

    Using a staging buffer allows the CPU to upload data once and then copy it into the final GPU-oriented allocation. 
    This is a common explicit upload pattern, although the optimal strategy can differ on hardware where CPU-visible and device-local memory are the same physical memory.

    Important Consequences:
    -Vertex/index buffers can reside in device-local memory.
    -Temporary staging resources can be destroyed after the transfer completes.
    -The same pattern can later support many static GPU-resource uploads.

    Source Mapping:
    Derived from the shared structure of the vertex/index buffer sections. 

### Block 46 — Vertex Buffer / `createVertexBuffer()`

    Purpose:
    Upload the engine's current CPU-side vertex vector into a device-local Vulkan Buffer that can be bound as graphics-pipeline vertex input.

    My Configuration:

    Calculate required buffer size:

    -bufferSize = sizeof(vertices[0]) * vertices.size()

    This calculates:
        size of one stored Vertex
            ×
        number of vertices
    giving the total number of bytes required to store the current vertices vector.

    The current implementation uses:
    -sizeof(vertices[0])

    rather than:
    -sizeof(Vertex)

    Create staging buffer:

    Call createBuffer():
    -size = bufferSize
    -usage = TransferSrc
    -memory properties:
        -HostVisible
        -HostCoherent
    Returned local RAII resources:
    -stagingBuffer
    -stagingBufferMemory

    Why these properties:

    HostVisible:
    -Allows the CPU to map the staging allocation.

    HostCoherent:
    -Allows ordinary CPU writes to mapped memory to become coherent without an explicit mapped-memory flush in this upload path.

    TransferSrc:
    -Allows stagingBuffer to act as the source of a Vulkan buffer-copy operation.

    Map staging memory:
    -data = stagingBufferMemory.mapMemory(0, bufferSize)

    Mapping:
    -starts at memory offset 0
    -covers bufferSize bytes
    `data` is a CPU pointer to the mapped staging-memory range.

    Copy CPU vertex data:
    -std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize))

    Source:
    -vertices.data()

    Destination:
    -mapped staging-buffer memory

    Size:
    -bufferSize bytes
    This copies the contiguous CPU-side contents of `vertices` into the staging allocation.

    Unmap:
    -stagingBufferMemory.unmapMemory()

    The staging buffer now contains the vertex bytes required for the GPU-side copy.

    Create final vertex buffer:

    Call createBuffer():
    -size = bufferSize
    -usage:
        -TransferDst
        -VertexBuffer
    -memory properties:
        -DeviceLocal
    Returned local RAII resources:
    -buffer
    -bufferMemory

    TransferDst:
    -Allows the final buffer to receive data through a buffer-copy operation.

    VertexBuffer:
    -Allows the final buffer to later be bound as a Vulkan vertex buffer.

    DeviceLocal:
    -Requests memory intended for efficient device/GPU access.

    Store persistent resources:
    -vertexBuffer = std::move(buffer)
    -vertexBufferMemory = std::move(bufferMemory)

    Ownership of the newly-created final Buffer and DeviceMemory wrappers is moved into the engine's persistent members.

    Copy staging data into final vertex buffer:

    Call:
    -copyBuffer(stagingBuffer, vertexBuffer, bufferSize)

    Current copyBuffer() behavior:
    -copy begins at source offset 0
    -copy begins at destination offset 0
    -exactly bufferSize bytes are copied
    -the copy is recorded into a temporary command buffer
    -the command buffer is submitted to graphicsQueue
    -graphicsQueue.waitIdle() is used before copyBuffer() returns

    Therefore the complete upload path is:
        vertices
        CPU vector
            ↓ memcpy
        stagingBuffer
        HostVisible | HostCoherent
        TransferSrc
            ↓ copyBuffer()
        vertexBuffer
        DeviceLocal
        TransferDst | VertexBuffer
            ↓
        later bound for rendering

    Staging-resource lifetime:

    stagingBuffer and stagingBufferMemory are local RAII objects.

    They are not moved into engine members.

    After copyBuffer() returns:
    -the current implementation has already waited for graphicsQueue to become idle
    -the transfer using the staging buffer has completed

    Therefore the local staging resources can safely be released when createVertexBuffer() finishes.

    Why:
    The engine's `vertices` vector exists in normal CPU memory.

    Vulkan rendering requires that vertex data be placed in a Vulkan Buffer created with VertexBuffer usage.

    The current renderer uses a staging upload because:
        CPU-side vertices
            ↓
        CPU-accessible staging memory
            ↓
        GPU buffer copy
            ↓
        device-local vertex buffer

    This separates:
    -memory convenient for CPU writes

    from:
    -memory intended for repeated GPU vertex reads.

    Important Consequences:
    -The current CPU `vertices` vector is uploaded into Vulkan-managed buffer memory.
    -The final `vertexBuffer` is created with both TransferDst and VertexBuffer usage.
    -The final backing memory is requested as DeviceLocal.
    -The CPU does not directly map the final device-local vertex-buffer allocation.
    -The temporary staging allocation is HostVisible and HostCoherent.
    -HostCoherent avoids the need for an explicit flush in the current mapped-write path.
    -The entire vertex array is copied beginning at offset 0.
    -vertexBuffer and vertexBufferMemory persist as engine members after this function returns.
    -stagingBuffer and stagingBufferMemory are temporary local RAII resources.
    -The current synchronous copyBuffer() implementation ensures the staging resources are no longer in use before they are destroyed.
    -The original CPU-side `vertices` vector is not moved or destroyed by this function; its bytes are copied into staging memory.

    // FUTURE ROBUSTNESS:
    // The current bufferSize expression uses:
    //     sizeof(vertices[0])
    //
    // This assumes the vertices vector is non-empty.
    //
    // If empty meshes may eventually reach this function, the engine should
    // explicitly handle that case before accessing vertices[0].
    //
    // An alternative size expression such as sizeof(Vertex) * vertices.size()
    // avoids indexing the vector merely to determine the element type's size.

    Source Mapping:
    createVertexBuffer() CPU-to-GPU vertex upload path.


### Block 47 — Index Buffer / `createIndexBuffer()`

    Purpose:
    Upload the engine's current CPU-side index vector into a device-local Vulkan Buffer that can later be bound for indexed drawing.


    My Configuration:

    Calculate required buffer size:

    -bufferSize = sizeof(indices[0]) * indices.size()

    This calculates:
        size of one stored index
            ×
        number of indices

    giving the total number of bytes required to store the current indices vector.

    The current implementation uses:
    -sizeof(indices[0])
    rather than explicitly naming the index type.

    Create staging buffer:

    Call createBuffer():
    -size = bufferSize
    -usage = TransferSrc
    -memory properties:
        -HostVisible
        -HostCoherent

    Returned local RAII resources:
    -stagingBuffer
    -stagingBufferMemory

    HostVisible:
    -Allows the CPU to map the staging allocation.

    HostCoherent:
    -Allows the current mapped CPU writes to be used without an explicit memory flush.

    TransferSrc:
    -Allows stagingBuffer to act as the source of a later Vulkan buffer-copy command.

    Map staging memory:
    -data = stagingBufferMemory.mapMemory(0, bufferSize)

    Mapping:
    -starts at offset 0
    -covers bufferSize bytes
    `data` becomes a CPU pointer to the mapped staging-memory range.

    Copy CPU index data:
    -std::memcpy(data, indices.data(), static_cast<size_t>(bufferSize))

    Source:
    -indices.data()

    Destination:
    -mapped staging-buffer memory

    Size:
    -bufferSize bytes

    This copies the contiguous CPU-side index data into the staging allocation.

    Unmap staging memory:
    -stagingBufferMemory.unmapMemory()

    The staging buffer now contains the index data required for the GPU-side transfer.

    Create final index buffer:

    Call createBuffer():
    -size = bufferSize
    -usage:
        -TransferDst
        -IndexBuffer
    -memory properties:
        -DeviceLocal

    Returned local RAII resources:
    -buffer
    -bufferMemory

    TransferDst:
    -Allows the final buffer to receive data from the staging buffer.

    IndexBuffer:
    -Allows the final buffer to later be bound as an index buffer.

    DeviceLocal:
    -Requests memory intended for efficient device/GPU access.

    Store persistent resources:
    -indexBuffer = std::move(buffer)
    -indexBufferMemory = std::move(bufferMemory)

    Ownership of the final Vulkan Buffer and DeviceMemory wrappers is moved into the engine's persistent members.

    Copy staging data into final index buffer:

    Call:
    -copyBuffer(stagingBuffer, indexBuffer, bufferSize)

    Current copyBuffer() behavior:
    -source offset = 0
    -destination offset = 0
    -copy size = bufferSize
    -records a temporary buffer-copy command
    -submits it to graphicsQueue
    -waits for graphicsQueue to become idle before returning

    Complete upload path:
        indices
        CPU vector
            ↓ memcpy
        stagingBuffer
        HostVisible | HostCoherent
        TransferSrc
            ↓ copyBuffer()
        indexBuffer
        DeviceLocal
        TransferDst | IndexBuffer
            ↓
        later bound for indexed drawing

    Staging-resource lifetime:

    stagingBuffer and stagingBufferMemory are local RAII resources.

    They are not stored in engine members.

    Because copyBuffer() currently waits for graphicsQueue to become idle before returning:
    -the transfer has completed
    -the staging resources are no longer needed by that copy

    They can therefore be released when createIndexBuffer() finishes.

    Why:
    The engine's `indices` vector exists in normal CPU memory.

    Indexed rendering requires those index values to be available through a Vulkan Buffer created with IndexBuffer usage.

    The staging upload separates:
        CPU-accessible memory
            ↓
        temporary staging buffer
            ↓
        GPU copy
            ↓
        device-local index buffer

    The index buffer can then be used by drawIndexed() to select vertices from the vertex buffer.

    Important Consequences:
    -The CPU-side `indices` vector is uploaded into Vulkan-managed buffer memory.
    -The final indexBuffer uses TransferDst | IndexBuffer usage.
    -Its backing memory is requested as DeviceLocal.
    -The CPU does not directly map the final device-local index-buffer allocation.
    -The temporary staging allocation is HostVisible and HostCoherent.
    -The entire current index array is copied beginning at offset 0.
    -indexBuffer and indexBufferMemory persist as engine members.
    -stagingBuffer and stagingBufferMemory exist only during this upload.
    -The current synchronous copyBuffer() implementation ensures staging resources are no longer in use before destruction.
    -The CPU-side indices vector remains intact; its bytes are copied rather than moved.
    -The resulting indexBuffer is later used together with vertexBuffer for indexed rendering.

    // FUTURE ROBUSTNESS:
    // The current bufferSize expression uses:
    //
    //     sizeof(indices[0])
    //
    // This assumes the indices vector is non-empty.
    //
    // If empty geometry may eventually reach this function, the engine should
    // handle that case before accessing indices[0].
    //
    // An expression based on the explicit index type could avoid indexing
    // the vector purely to determine element size.

    Source Mapping:
    createIndexBuffer() CPU-to-GPU index upload path.


### Block 48 — Uniform Buffers / `createUniformBuffers()`

    Purpose:
    Create one persistently-mapped Vulkan uniform buffer for each frame-in-flight so CPU-side uniform data can be updated directly for the frame currently being prepared.

    My Configuration:

    Calculate uniform-buffer size:
    -bufferSize = sizeof(UniformBufferObject)

    Each uniform buffer therefore has enough storage for exactly one UniformBufferObject.

    Create one buffer per frame-in-flight:

    Loop:
    -i = 0 -> MAX_FRAMES_IN_FLIGHT - 1

    For every frame slot, call createBuffer():
    -size = bufferSize
    -usage = UniformBuffer
    -memory properties:
        -HostVisible
        -HostCoherent

    Returned local RAII resources:
    -buffer
    -bufferMemory

    UniformBuffer usage:
    -Allows the Vulkan Buffer to later be referenced through a uniform-buffer descriptor.

    HostVisible:
    -Allows CPU code to map and directly write to the allocation.

    HostCoherent:
    -Allows ordinary mapped CPU writes to be used without requiring an explicit mapped-memory flush in the current update path.

    Store buffer:
    -uniformBuffers.emplace_back(std::move(buffer))

    Ownership of the newly-created vk::raii::Buffer is moved into:
    -uniformBuffers

    Each vector entry therefore corresponds to one frame-in-flight slot.

    Store backing memory:
    -uniformBuffersMemory.emplace_back(std::move(bufferMemory))

    Ownership of the associated vk::raii::DeviceMemory is moved into:
    -uniformBuffersMemory

    Persistently map memory:

    Call:
    -uniformBuffersMemory.back().mapMemory(0, bufferSize)

    Mapping:
    -offset = 0
    -size = bufferSize

    Store the returned CPU pointer in:
    -uniformBuffersMapped

    through:
    -uniformBuffersMapped.emplace_back(...)

    This produces matching per-frame vectors:
        frame 0
        ├── uniformBuffers[0]
        ├── uniformBuffersMemory[0]
        └── uniformBuffersMapped[0]

        frame 1
        ├── uniformBuffers[1]
        ├── uniformBuffersMemory[1]
        └── uniformBuffersMapped[1]

        ...

        frame N
        ├── Buffer
        ├── backing DeviceMemory
        └── mapped CPU pointer


    Persistent mapping:

    Unlike the temporary staging buffers used for vertex and index uploads, these uniform-buffer allocations are not immediately unmapped.

    Their mapped pointers remain stored in:
    -uniformBuffersMapped

    This allows later code to update each frame's UniformBufferObject directly through the existing mapped pointer.

    Why:
    Uniform data changes frequently during rendering.

    Examples include values such as:
    -model transforms
    -view matrices
    -projection matrices

    Repeatedly mapping and unmapping memory for every frame update would add unnecessary work.

    Instead, WizEngine creates and maps each frame's uniform buffer once:
        create buffer
            ↓
        map memory
            ↓
        keep pointer
            ↓
        update through pointer each frame


    One buffer is created for each frame-in-flight so the CPU does not need to overwrite the same uniform-buffer memory while a previous frame may still be using it on the GPU.

    Conceptually:
        frameIndex 0
            ↓
        uniformBuffersMapped[0]
            ↓
        uniformBuffers[0]

        frameIndex 1
            ↓
        uniformBuffersMapped[1]
            ↓
        uniformBuffers[1]

    Important Consequences:
    -There is one UniformBufferObject-sized Vulkan Buffer per frame-in-flight.
    -Each buffer uses UniformBuffer usage.
    -Each allocation is HostVisible and HostCoherent.
    -No staging buffer is used for uniform-buffer creation.
    -No DeviceLocal-only destination buffer is created here.
    -No copyBuffer() operation occurs.
    -Each buffer's memory is mapped once during initialization.
    -The mapped pointers are stored for later CPU-side updates.
    -The buffers remain persistently mapped after createUniformBuffers() returns.
    -uniformBuffers, uniformBuffersMemory, and uniformBuffersMapped are intended to remain index-aligned by frame slot.
    -The frameIndex can later select the matching buffer, memory allocation, and mapped pointer for the current frame.
    -HostCoherent avoids an explicit flush requirement for the current mapped writes, but it does not provide CPU/GPU execution synchronization.
    -Per-frame synchronization still determines when a frame slot's uniform memory is safe to overwrite.

    Source Mapping:
    createUniformBuffers() per-frame persistent uniform-buffer creation and mapping.


# Descriptor Allocation and Population

### Block 49 — Descriptor Pool / `createDescriptorPool()`

    Purpose:
    Create the descriptor-allocation pool with enough capacity to allocate the renderer's current per-frame descriptor sets and the descriptor types those sets require.

    My Configuration:

    Create descriptor-pool size array:
    -std::array<vk::DescriptorPoolSize, 2> poolSizes

    The array contains two descriptor-type capacity entries.

    Pool Size 1 — Uniform Buffer descriptors:
    -type = UniformBuffer
    -descriptorCount = MAX_FRAMES_IN_FLIGHT

    This reserves capacity for:
        MAX_FRAMES_IN_FLIGHT
        UniformBuffer descriptors

    Pool Size 2 — Combined Image Sampler descriptors:
    -type = CombinedImageSampler
    -descriptorCount = MAX_FRAMES_IN_FLIGHT

    This reserves capacity for:
        MAX_FRAMES_IN_FLIGHT
        CombinedImageSampler descriptors

    Important distinction:
    descriptorCount refers to the number of individual descriptors of that type the pool can allocate.

    It does NOT directly mean:
    -number of descriptor sets

    The descriptor-set limit is configured separately through:
    -maxSets

    Current intended relationship:

    Each frame's descriptor set currently requires:
        1 × UniformBuffer descriptor
        1 × CombinedImageSampler descriptor

    Therefore with:
        MAX_FRAMES_IN_FLIGHT = N
    the pool reserves:
        N UniformBuffer descriptors
        N CombinedImageSampler descriptors
        N total descriptor sets

    This exactly matches the current one-set-per-frame design.

    Create DescriptorPoolCreateInfo:
    -flags = FreeDescriptorSet
    -maxSets = MAX_FRAMES_IN_FLIGHT
    -poolSizeCount = static_cast<uint32_t>(poolSizes.size())
    -pPoolSizes = poolSizes.data()

    flags = FreeDescriptorSet:

    Enables descriptor sets allocated from this pool to be individually freed.

    Without this flag, individual descriptor-set freeing through the corresponding Vulkan free operation would not be enabled for this pool.

    The current source enables that capability even though this function itself does not allocate or free any descriptor sets.

    maxSets:
    -maxSets = MAX_FRAMES_IN_FLIGHT

    This limits the total number of descriptor sets that can be allocated from this pool to the configured frame count.

    poolSizeCount:
    -poolSizes.size() = 2

    This tells Vulkan that two DescriptorPoolSize entries are being supplied.

    pPoolSizes:
    -pPoolSizes = poolSizes.data()

    This points DescriptorPoolCreateInfo to the beginning of the two-element poolSizes array.

    Create descriptor pool:
    -descriptorPool = vk::raii::DescriptorPool(device, poolInfo)

    Inputs:
    -logical device
    -DescriptorPoolCreateInfo

    Store resulting RAII object in:
    -descriptorPool

    Why:
    Descriptor sets cannot simply be created independently without allocation backing.

    Vulkan descriptor sets are allocated from a DescriptorPool.

    The pool establishes two related but separate capacities:
        DescriptorPool
        ├── maximum descriptor-set count
        └── descriptor capacity by type

    In WizEngine's current design:
        DescriptorPool
        │
        ├── MAX_FRAMES_IN_FLIGHT
        │   UniformBuffer descriptors
        │
        ├── MAX_FRAMES_IN_FLIGHT
        │   CombinedImageSampler descriptors
        │
        └── MAX_FRAMES_IN_FLIGHT
            maximum descriptor sets

    The pool does NOT contain:
    -the actual UniformBufferObject data
    -texture pixels
    -the texture ImageView
    -the Sampler itself

    Instead, it provides allocation capacity for descriptor sets that will later reference those resources.

    Current relationship:
        Descriptor Set Layout
        defines required bindings
                ↓
        Descriptor Pool
        supplies allocation capacity
                ↓
        Descriptor Sets
        are allocated
                ↓
        Descriptor Sets are populated with:
        ├── uniformBuffers[i]
        └── textureImageView + textureSampler

    Important Consequences:
    -The descriptor pool supports at most MAX_FRAMES_IN_FLIGHT descriptor sets.
    -It contains capacity for MAX_FRAMES_IN_FLIGHT UniformBuffer descriptors.
    -It contains capacity for MAX_FRAMES_IN_FLIGHT CombinedImageSampler descriptors.
    -Descriptor-type capacity and descriptor-set capacity are separate limits.
    -The current capacities exactly match one UBO descriptor and one combined-image-sampler descriptor per frame descriptor set.
    -There is currently no spare descriptor capacity beyond that intended configuration.
    -FreeDescriptorSet enables individual descriptor sets from this pool to be freed.
    -The descriptor pool does not contain the actual resources referenced by descriptors.
    -Changing the descriptor-set layout to require additional descriptors may require increasing the corresponding pool capacities.
    -Increasing the number of per-frame descriptor sets may require increasing maxSets and the descriptor counts.
    -descriptorPool persists as an engine member for later descriptor-set allocation.

    // CURRENT CAPACITY MODEL:
    //
    // For N = MAX_FRAMES_IN_FLIGHT:
    //
    //     max descriptor sets = N
    //
    //     UniformBuffer descriptors = N
    //
    //     CombinedImageSampler descriptors = N
    //
    // With the current descriptor layout requiring one of each type per set,
    // this is sufficient for exactly N complete per-frame descriptor sets.

    Source Mapping:
    createDescriptorPool() descriptor allocation-capacity configuration.


### Block 50 — Descriptor Sets / `createDescriptorSets()`

    Purpose:
    Allocate one descriptor set for each frame-in-flight and populate each set with the concrete resources required by the current descriptor-set layout.

    Current layout contract:
        binding 0 -> UniformBuffer
        binding 1 -> CombinedImageSampler

    My Configuration:

    Step 1 — Create descriptor-set layout list:
    Create:
    -std::vector<vk::DescriptorSetLayout> layouts(
        MAX_FRAMES_IN_FLIGHT,
        *descriptorSetLayout
    )

    This creates a vector containing:
        MAX_FRAMES_IN_FLIGHT

    copies of the same descriptorSetLayout handle.

    Therefore every descriptor set allocated here uses the same binding structure:
        binding 0 -> UniformBuffer
        binding 1 -> CombinedImageSampler


    Step 2 — Describe descriptor-set allocation:
    Create:
    -vk::DescriptorSetAllocateInfo allocInfo

    Configure:
    -descriptorPool = descriptorPool
        Allocate the sets from the descriptor pool created previously.

    -descriptorSetCount = static_cast<uint32_t>(layouts.size())
        Request one descriptor set for every entry in layouts.

        Since layouts.size() == MAX_FRAMES_IN_FLIGHT:
            descriptorSetCount = MAX_FRAMES_IN_FLIGHT

    -pSetLayouts = layouts.data()
        Points Vulkan to the array of descriptor-set layouts that should be used for the allocated sets.

    Step 3 — Allocate descriptor sets:
    -descriptorSets = device.allocateDescriptorSets(allocInfo)

    The resulting descriptor sets are stored in:
    -descriptorSets

    Current relationship:
        descriptorSets[0]
            -> descriptorSetLayout
        descriptorSets[1]
            -> descriptorSetLayout

        ...

        descriptorSets[MAX_FRAMES_IN_FLIGHT - 1]
            -> descriptorSetLayout

    At this point, the sets have been allocated with the correct layout, but their bindings still need to be populated with actual resources.


    Step 4 — Populate each frame's descriptor set:

    Loop: i = 0 -> MAX_FRAMES_IN_FLIGHT - 1

    Buffer descriptor information:

    Create:
    -vk::DescriptorBufferInfo bufferInfo

    Configure:
    -buffer = uniformBuffers[i]
        Each frame's descriptor set references that frame's own Uniform Buffer.
    -offset = 0
        The descriptor begins at byte 0 of the uniform buffer.
    -range = sizeof(UniformBufferObject)
        The descriptor exposes one complete UniformBufferObject-sized range.

    Therefore:
        descriptorSets[i]
            ↓ binding 0
        uniformBuffers[i]

    Image descriptor information:

    Create:
    -vk::DescriptorImageInfo imageInfo

    Configure:
    -sampler = textureSampler
    -imageView = textureImageView
    -imageLayout = ShaderReadOnlyOptimal

    The combined image-sampler descriptor therefore references:
        textureImageView
            +
        textureSampler

    and declares that the referenced texture image will be in:
    -ShaderReadOnlyOptimal

    Unlike the uniform buffer, these texture resources are currently shared by every frame.

    Step 5 — Create descriptor writes:

    Create:
    -std::array<vk::WriteDescriptorSet, 2> descriptorWrites

    One write populates binding 0. The second write populates binding 1.

    Write 1 — Uniform Buffer:

    Destination:
    -dstSet = descriptorSets[i]
    -dstBinding = 0
    -dstArrayElement = 0

    Descriptor:
    -descriptorCount = 1
    -descriptorType = UniformBuffer
    -pBufferInfo = &bufferInfo

    Meaning:
        descriptorSets[i]
            binding 0
                ↓
        uniformBuffers[i]

    dstArrayElement = 0:
    The current binding contains one descriptor, so the write begins at descriptor-array element 0.

    Write 2 — Combined Image Sampler:

    Destination:
    -dstSet = descriptorSets[i]
    -dstBinding = 1
    -dstArrayElement = 0

    Descriptor:
    -descriptorCount = 1
    -descriptorType = CombinedImageSampler
    -pImageInfo = &imageInfo

    Meaning:
        descriptorSets[i]
           binding 1
              ↓
        textureImageView
              +
        textureSampler


    Step 6 — Update the descriptor set:

    Call:

    -device.updateDescriptorSets(descriptorWrites, {})

    First argument:
    -descriptorWrites

    Contains the two WriteDescriptorSet operations that populate bindings 0 and 1.

    Second argument:
    -{}

    No descriptor-copy operations are performed.

    Therefore this call updates the current descriptorSets[i] using new descriptor writes rather than copying descriptors from another descriptor set.

    Complete per-frame relationship:
        descriptorSets[i]
        │
        ├── binding 0
        │      ↓
        │   uniformBuffers[i]
        │
        └── binding 1
               ↓
          textureImageView
               +
          textureSampler

    For example, with two frames-in-flight:

        descriptorSets[0]
        ├── binding 0 -> uniformBuffers[0]
        └── binding 1 -> shared texture

        descriptorSets[1]
        ├── binding 0 -> uniformBuffers[1]
        └── binding 1 -> shared texture

    Why:
    The DescriptorSetLayout only defines the resource-binding contract.

    It establishes:
        "binding 0 expects a UniformBuffer"
        "binding 1 expects a CombinedImageSampler"

    It does not specify which actual resources should occupy those bindings.

    createDescriptorSets() performs that second step:
        DescriptorSetLayout
            ↓ defines structure
        Descriptor Set
            ↓ allocated from pool
        WriteDescriptorSet
            ↓ supplies actual resources
        Populated Descriptor Set

    Each frame receives its own descriptor set because each frame has its own Uniform Buffer.

    This allows:
        frameIndex
            ↓
        descriptorSets[frameIndex]
            ↓
        uniformBuffers[frameIndex]
    while the current texture ImageView and Sampler can be shared across all frame descriptor sets.

    Important Consequences:
    -Exactly MAX_FRAMES_IN_FLIGHT descriptor sets are requested.
    -Every set uses the same descriptorSetLayout.
    -Every set contains two populated bindings.
    -Binding 0 references that frame's specific uniformBuffers[i].
    -Binding 1 references the same textureImageView and textureSampler for every frame.
    -The Uniform Buffer descriptor begins at offset 0.
    -Its range is exactly sizeof(UniformBufferObject).
    -Both writes begin at descriptor-array element 0.
    -Each binding currently contains exactly one descriptor.
    -The image descriptor expects the texture image to be in ShaderReadOnlyOptimal.
    -No descriptor-copy operations are performed.
    -device.updateDescriptorSets() performs two descriptor writes for each frame.
    -Binding the correct descriptorSets[frameIndex] later gives the shaders access to that frame's current uniform data and the shared texture.

    Source Mapping:
    createDescriptorSets() descriptor-set allocation and resource-binding population.


# Frame Command Infrastructure

### Block 51 — Command Buffers / `createCommandBuffers()`

    Purpose:
    Allocate one reusable primary graphics command buffer for each frame-in-flight slot.

    My Configuration:

    Create:
    -vk::CommandBufferAllocateInfo allocInfo

    Command pool:
    -commandPool = commandPool

    Command buffers are allocated from the engine's existing command pool.

    Because commandPool was created for graphicsIndex, these command buffers are associated with that queue family.

    Command-buffer level:
    -level = Primary

    Primary command buffers can be submitted directly to a compatible Vulkan queue.

    This differs from secondary command buffers, which are intended to be executed from another command buffer rather than submitted independently in the same way.

    Command-buffer count:
    -commandBufferCount = MAX_FRAMES_IN_FLIGHT

    Therefore WizEngine allocates one command buffer per frame-in-flight slot

    Example with two frames-in-flight:
        frameIndex 0
            ↓
        commandBuffers[0]

        frameIndex 1
            ↓
        commandBuffers[1]

    Allocate and store:
    -commandBuffers = vk::raii::CommandBuffers(device, allocInfo)

    Inputs:
    -logical device
    -CommandBufferAllocateInfo

    The allocated RAII command buffers are stored in:
    -commandBuffers

    Why:
    Vulkan rendering commands are first recorded into command buffers.

    Those recorded commands can include operations such as:
    -image layout transitions
    -beginning dynamic rendering
    -binding the graphics pipeline
    -binding vertex/index buffers
    -binding descriptor sets
    -setting dynamic state
    -issuing draw commands
    -ending rendering

    Recording a command does not immediately execute it.

    Broadly:
        CPU records commands
            ↓
        CommandBuffer
            ↓ submit
        graphicsQueue
            ↓
        GPU executes commands

    WizEngine uses separate command buffers for each frame-in-flight so each reusable frame slot has its own command-recording resource.

    Runtime synchronization later determines when a particular frame slot's command buffer is safe to reset and reuse.

    Command-buffer reuse:

    The command pool was created with:
    -ResetCommandBuffer

    Therefore individual command buffers allocated from that pool can later be reset and re-recorded.

    The correct runtime model is:
        wait for frame fence
            ↓
        previous GPU submission for that frame slot has completed
            ↓
        reset commandBuffers[frameIndex]
            ↓
        record new frame commands
            ↓
        submit command buffer again

    The command buffer itself is not what the CPU waits on.

    The frame's Fence provides the GPU-to-CPU completion synchronization used to determine when reuse is safe.

    Important Consequences:
    -Exactly MAX_FRAMES_IN_FLIGHT command buffers are allocated.
    -Each frame-in-flight slot has its own command buffer.
    -All allocated command buffers are Primary.
    -Primary command buffers can be submitted directly to a compatible queue.
    -The command buffers are allocated from commandPool.
    -Their queue-family association therefore comes from commandPool's graphicsIndex configuration.
    -The command buffers are stored as Vulkan-Hpp RAII objects in commandBuffers.
    -Commands are recorded now/later but execute only after queue submission.
    -The command pool's ResetCommandBuffer flag allows individual command buffers to be reset before re-recording.
    -Safe reuse is controlled by synchronization such as the per-frame fence; the command buffer itself is not waited on.

    Source Mapping:
    createCommandBuffers() per-frame primary command-buffer allocation.


### Block 52 — Synchronization Model / `createSyncObjects()`

    Purpose:
    Create the semaphores and fences used by the runtime frame path to coordinate swapchain image acquisition, graphics submission, presentation, and safe reuse of per-frame resources.

    My Configuration:

    Step 1 — Verify synchronization vectors are initially empty:

    Assert:
    -presentCompleteSemaphore.empty()
    -renderFinishedSemaphore.empty()
    -inFlightFences.empty()

    through:
    -assert(
        presentCompleteSemaphore.empty() &&
        renderFinishedSemaphore.empty() &&
        inFlightFences.empty())

    Purpose:
    -Detect accidental attempts to append another set of synchronization objects to already-populated vectors.

    Important:
    -`assert()` is primarily a debug-time correctness check.
    -If assertions are disabled in the build, this does not itself perform runtime cleanup or recovery.

    Step 2 — Create render-finished semaphores:

    Loop: i = 0 -> swapChainImages.size() - 1

    For every swapchain image:

    Create:
    -vk::raii::Semaphore

    Using:
    -device
    -vk::SemaphoreCreateInfo{}

    Store in:
    -renderFinishedSemaphore

    Therefore:
        renderFinishedSemaphore.size() = swapChainImages.size()

    Current indexing model:
        swapchain image 0
            ↓
        renderFinishedSemaphore[0]

        swapchain image 1
            ↓
        renderFinishedSemaphore[1]

        ...

        swapchain image N
            ↓
        renderFinishedSemaphore[N]

    These semaphores are intentionally allocated per swapchain image rather than per frame-in-flight.

    Step 3 — Create per-frame acquisition semaphores:

    Loop: i = 0 -> MAX_FRAMES_IN_FLIGHT - 1

    Create:
    -vk::raii::Semaphore

    Using:
    -device
    -vk::SemaphoreCreateInfo{}

    Store in:
    -presentCompleteSemaphore

    Therefore:
    presentCompleteSemaphore.size() = MAX_FRAMES_IN_FLIGHT

    Step 4 — Create per-frame fences:

    Inside the same MAX_FRAMES_IN_FLIGHT loop:

    Create:
    -vk::raii::Fence

    Fence configuration:
    -flags = Signaled

    Store in:
    -inFlightFences

    Therefore:
    inFlightFences.size() = MAX_FRAMES_IN_FLIGHT

    Current indexing model:
        frameIndex 0
            ↓
        inFlightFences[0]

        frameIndex 1
            ↓
        inFlightFences[1]

        ...

    Fence initial state:

    Each fence is created with:

    -vk::FenceCreateFlagBits::eSignaled

    This means each fence begins in the signaled state.

    Why:
    The runtime frame loop waits on the current frame's fence before reusing that frame slot.

    On the very first frame, no previous GPU submission exists for that slot.

    Starting the fence signaled allows the first fence wait to succeed immediately rather than waiting for a submission that has never occurred.

    Current resource-count relationship:
        Swapchain-dependent synchronization:

        swapChainImages.size()
            ↓
        renderFinishedSemaphore[], one per swapchain image

        Frame-in-flight synchronization:

        MAX_FRAMES_IN_FLIGHT
            ↓
        presentCompleteSemaphore[], one per frame slot

        MAX_FRAMES_IN_FLIGHT
            ↓
        inFlightFences[], one per frame slot

    This is intentionally different from:
        swapChainImages.size()

    which does not necessarily equal:
        MAX_FRAMES_IN_FLIGHT

    Why:
    The renderer has two separate identities active during a frame:

        frameIndex = reusable CPU/GPU frame slot

        imageIndex = swapchain image acquired for this frame

    These are not interchangeable.

    The synchronization arrays reflect that distinction.

    Runtime relationship:
        frameIndex
        │
        ├── presentCompleteSemaphore[frameIndex]
        └── inFlightFences[frameIndex]

        imageIndex
        │
        └── renderFinishedSemaphore[imageIndex]

    Broad synchronization flow:

        wait for inFlightFences[frameIndex]
            ↓
        frame slot is safe to reuse
            ↓
        acquire swapchain image
            ↓
        acquisition signals presentCompleteSemaphore[frameIndex]
            ↓
        graphics submission waits on acquisition semaphore
            ↓
        rendering completes
            ↓
        graphics submission signals renderFinishedSemaphore[imageIndex]
            ↓
        presentation waits on render-finished semaphore

    Important Consequences:
    -Synchronization vectors are expected to be empty before initial creation.
    -renderFinishedSemaphore contains one semaphore per swapchain image.
    -presentCompleteSemaphore contains one semaphore per frame-in-flight.
    -inFlightFences contains one fence per frame-in-flight.
    -renderFinishedSemaphore is indexed using imageIndex in the current synchronization design.
    -presentCompleteSemaphore and inFlightFences are indexed using frameIndex.
    -frameIndex and imageIndex represent different things and may contain different numeric values.
    -The number of swapchain images is independent from MAX_FRAMES_IN_FLIGHT.
    -Each fence starts signaled so the first frame does not wait for nonexistent previous work.
    -The semaphores are created with default SemaphoreCreateInfo.
    -No queue submission, waiting, resetting, or signaling occurs inside createSyncObjects(); this function only creates the synchronization objects.
    -Because renderFinishedSemaphore depends on swapChainImages.size(), its required count is tied to the current swapchain image count.

    // NAMING NOTE:
    // presentCompleteSemaphore is currently the source-code name.
    //
    // Its runtime role is the semaphore associated with swapchain-image
    // acquisition before rendering, so a future name such as
    // imageAvailableSemaphore may describe its role more clearly.
    //
    // This is only a naming improvement; the current synchronization
    // architecture does not require changing for that reason alone.

    Source Mapping:
    createSyncObjects() synchronization-object creation and indexing model.


# Runtime

## Runtime Rendering Helpers

### Block 53 — Per-Frame Uniform Update / `updateUniformBuffer()`

    Purpose:
    Build the current frame's model, view, and projection matrices and copy the completed UniformBufferObject directly into the persistently-mapped uniform buffer associated with the current frame-in-flight slot.

    My Configuration:

    Function input:
    -currentFrame
    This identifies which per-frame Uniform Buffer should receive the updated data.

    It is later used to select:
    -uniformBuffersMapped[currentFrame]

    Step 1 — Establish animation start time:
    -static auto startTime =
        std::chrono::high_resolution_clock::now()
    `startTime` is declared static.

    Therefore it is initialized only once, on the first call to updateUniformBuffer(), and retains that original value across later calls.

    Conceptually:
        first updateUniformBuffer() call
            ↓
        startTime recorded once
            ↓
        future calls reuse same startTime

    Step 2 — Get current time:
    -currentTime = std::chrono::high_resolution_clock::now()

    Unlike startTime, currentTime is recalculated each time the function runs.

    Step 3 — Calculate elapsed time:
    -time = duration<float, seconds::period>(currentTime - startTime).count()

    Result:
    -floating-point number of elapsed seconds since the first call to updateUniformBuffer()

    This elapsed time is used to animate the model rotation.

    Step 4 — Create temporary UniformBufferObject:
    -UniformBufferObject ubo{}

    This creates the CPU-side UBO structure that will hold:
    -model matrix
    -view matrix
    -projection matrix

    The structure is filled locally first and copied into mapped Vulkan memory after all matrices have been calculated.

    Step 5 — Build model matrix:

    Start from identity matrix:
    -glm::mat4(1.0f)

    Rotate by:
    -time * glm::radians(90.0f)

    Rotation axis:
    -{0.0f, 0.0f, 1.0f}

    Store in:
    -ubo.model

    Current model transformation:
        identity matrix
            ↓
        rotate around +Z axis
            ↓
        ubo.model

    Because the angle is:
        elapsedSeconds × 90 degrees

    the model currently rotates at approximately:
        90 degrees per second
    around the Z axis.

    Step 6 — Build view matrix:

    Call:
    -glm::lookAt(...)

    Camera position:
    -{2.0f, 2.0f, 2.0f}

    Camera target:
    -{0.0f, 0.0f, 0.0f}

    Up direction:
    -{0.0f, 0.0f, 1.0f}

    Store in:
    -ubo.view

    Conceptually:
        camera position
        (2, 2, 2)
            ↓ looks toward
        origin
        (0, 0, 0)
    with:
        +Z
    treated as the camera's world-up direction.

    The view matrix therefore describes the current camera orientation and position relative to the scene.

    Step 7 — Build projection matrix:

    Call:
    -glm::perspective(...)

    Field of view:
    -glm::radians(45.0f) = 45-degree vertical field of view

    Aspect ratio:
    -static_cast<float>(swapChainExtent.width)
        /
    static_cast<float>(swapChainExtent.height)

    The projection therefore adapts to the current swapchain dimensions.

    Near clipping plane:
    -0.1f

    Far clipping plane:
    -10.0f

    Store result in:
    -ubo.proj

    Current projection configuration:
        FOV = 45°
        aspect = swapchain width / height
        near = 0.1
        far = 10.0


    Step 8 — Copy UBO into current frame's mapped memory:

    Call:
    -std::memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo))

    Destination:
    -uniformBuffersMapped[currentFrame]
    This is the persistently-mapped CPU pointer corresponding to the current frame-in-flight's Uniform Buffer.

    Source:
    -&ubo
    This points to the completed temporary UniformBufferObject.

    Size:
    -sizeof(ubo)
    Therefore one complete UniformBufferObject is copied.

    Current relationship:

        CPU calculates
        UniformBufferObject
        ├── model
        ├── view
        └── projection
            ↓ memcpy
        uniformBuffersMapped[currentFrame]
            ↓ backs
        uniformBuffers[currentFrame]
            ↓ descriptor binding 0
        vertex shader

    No staging buffer is involved.
    No copyBuffer() operation is involved.
    No Vulkan transfer command is recorded.

    The CPU writes directly into the memory that was persistently mapped during createUniformBuffers().

    Why:
    The transformation matrices change at runtime.

    In the current implementation:

    -model changes continuously because it rotates over time
    -view currently uses a fixed camera
    -projection depends on the swapchain aspect ratio

    Each frame therefore creates a fresh UniformBufferObject and updates the Uniform Buffer for that frame slot.

    Because each frame-in-flight has its own Uniform Buffer:
        frameIndex 0
            ↓
        uniformBuffersMapped[0]

        frameIndex 1
            ↓
        uniformBuffersMapped[1]

    the CPU can update the current frame's data without intentionally overwriting another frame slot's uniform data.

    Synchronization relationship:

    updateUniformBuffer(currentFrame) assumes the current frame slot is safe for CPU reuse.

    That safety is established elsewhere in the frame loop by waiting for:
    -inFlightFences[currentFrame]
    before reusing that frame's resources.

    The Uniform Buffer memory itself being HostCoherent does NOT provide that CPU/GPU execution synchronization.

    HostCoherent handles memory visibility requirements for the mapped write path; the Fence handles safe timing of resource reuse.

    Important Consequences:
    -startTime is initialized only once and persists between calls.
    -time represents elapsed seconds since the first uniform update.
    -The model currently rotates approximately 90° per second around the +Z axis.
    -The camera is positioned at (2, 2, 2).
    -The camera looks toward the world origin.
    -The camera uses +Z as its up direction.
    -The projection uses a 45° field of view.
    -Projection aspect ratio is derived from the current swapChainExtent.
    -The near clipping plane is 0.1.
    -The far clipping plane is 10.0.
    -One temporary UniformBufferObject is constructed on the CPU each call.
    -The completed UBO is copied directly into uniformBuffersMapped[currentFrame].
    -No staging buffer or GPU transfer command is required.
    -The descriptor set for the same frame later exposes uniformBuffers[currentFrame] to the shader.
    -Safe per-frame buffer reuse depends on the synchronization performed elsewhere in drawFrame().

    // SOURCE NOTE:
    // This function does not contain any additional projection-matrix
    // Y-axis adjustment after glm::perspective().
    //
    // If WizEngine requires such a coordinate-convention adjustment,
    // it must therefore be handled elsewhere in the current renderer
    // configuration or added here later.
    //
    // This note does not by itself establish that the current output is incorrect.


    // FUTURE ROBUSTNESS:
    // currentFrame is assumed to be a valid index into:
    //
    //     uniformBuffersMapped
    //
    // swapChainExtent.height is also assumed to be nonzero before
    // calculating the projection aspect ratio.

    Source Mapping:
    updateUniformBuffer() per-frame transformation calculation and direct mapped Uniform Buffer update.

### Block 54 — Command Recording / `recordCommandBuffer()`

    Purpose:
    Record all GPU graphics commands required to render the current frame into the acquired swapchain image.

    This function connects:
    -the current frame-in-flight resources selected by frameIndex
    -the acquired swapchain image selected by imageIndex

    Function input:
    -imageIndex

    imageIndex identifies:
    -swapChainImages[imageIndex]
    -swapChainImageViews[imageIndex]

    The command buffer itself is selected separately using:
    -frameIndex

    Current indexing relationship:
        frameIndex
        ├── commandBuffers[frameIndex]
        └── descriptorSets[frameIndex]

        imageIndex
        ├── swapChainImages[imageIndex]
        └── swapChainImageViews[imageIndex]

    Step 1 — Select current frame's command buffer:
    -auto &commandBuffer = commandBuffers[frameIndex]

    This creates a reference to the command buffer belonging to the current frame-in-flight slot.
    The command buffer is not selected using imageIndex.

    Step 2 — Begin command-buffer recording:
    -commandBuffer.begin({})

    A default CommandBufferBeginInfo configuration is used.

    No OneTimeSubmit flag is supplied here.

    This begins normal recording of the current frame's reusable primary command buffer.

    Step 3 — Transition acquired swapchain image for rendering:

    Call transitionImageLayout() for:
    -image = swapChainImages[imageIndex]

    Transition:
    -oldLayout = Undefined
    -newLayout = ColorAttachmentOptimal

    Source access:
    -none

    Destination access:
    -ColorAttachmentWrite

    Source stage:
    -ColorAttachmentOutput

    Destination stage:
    -ColorAttachmentOutput

    Aspect:
    -Color

    This prepares the acquired swapchain image for use as the resolve destination of the color attachment.

    Using Undefined as the old layout means previous image contents are not preserved.

    Step 4 — Transition multisampled color image:

    Call transitionImageLayout() for:
    -colorImage

    Transition:
    -Undefined
        ->
    -ColorAttachmentOptimal

    Access:
    -source = ColorAttachmentWrite
    -destination = ColorAttachmentWrite

    Stages:
    -source = ColorAttachmentOutput
    -destination = ColorAttachmentOutput

    Aspect:
    -Color

    colorImage is the multisampled intermediate color attachment.

    It receives the actual rasterized MSAA color samples before they are resolved into the single-sampled swapchain image.

    Step 5 — Transition depth image:

    Call transitionImageLayout() for:
    -depthImage

    Transition:
    -Undefined
        ->
    -DepthAttachmentOptimal

    Access:
    -source = DepthStencilAttachmentWrite
    -destination = DepthStencilAttachmentWrite

    Stages:
    -EarlyFragmentTests | LateFragmentTests
        ->
    -EarlyFragmentTests | LateFragmentTests

    Aspect:
    -Depth

    This prepares depthImage for use as the depth attachment during rendering.

    Step 6 — Define attachment clear values:

    Color clear:
    -{0.0f, 0.0f, 0.0f, 1.0f}

    Meaning approximately:
        R = 0
        G = 0
        B = 0
        A = 1
    Result:
    -opaque black

    Depth/stencil clear:

    -depth = 1.0f
    -stencil = 0

    With the current depth comparison operation of Less, clearing depth to 1.0 represents the far end of the normal depth range so nearer fragments can pass the depth test.

    Step 7 — Configure multisampled color attachment:

    Create:
    -vk::RenderingAttachmentInfo colorAttachmentInfo

    Main attachment:

    -imageView = colorImageView
    -imageLayout = ColorAttachmentOptimal

    Therefore rendering writes color into:
    -the multisampled colorImage

    Resolve configuration:
    -resolveMode = Average

    -resolveImageView = swapChainImageViews[imageIndex]

    -resolveImageLayout = ColorAttachmentOptimal

    This produces the relationship:

        rasterized fragments
            ↓
        multisampled colorImage
            ↓ Average resolve
        swapChainImage[imageIndex]

    eAverage combines the multisample values when resolving them into the single-sampled swapchain image.

    Attachment operations:
    -loadOp = Clear
    -storeOp = Store
    -clearValue = clearColor

    loadOp = Clear:
    -Rendering begins with the MSAA color attachment cleared to black.

    storeOp = Store:
    -The multisampled attachment is configured to preserve its resulting contents after the rendering operation.

    The resolved swapchain image receives the single-sampled output used for presentation.

    Step 8 — Configure depth attachment:

    Create:
    -vk::RenderingAttachmentInfo depthAttachmentInfo

    Configuration:
    -imageView = depthImageView
    -imageLayout = DepthAttachmentOptimal
    -loadOp = Clear
    -storeOp = DontCare
    -clearValue = clearDepth

    loadOp = Clear:
    -Depth starts at 1.0 for this rendering operation.

    storeOp = DontCare:
    -The renderer does not require the depth attachment's contents to be preserved after rendering completes.

    Step 9 — Configure dynamic rendering:

    Create:
    -vk::RenderingInfo renderingInfo

    Render area:
    -offset = {0, 0}
    -extent = swapChainExtent

    Rendering therefore covers the current swapchain extent.

    Layers:
    -layerCount = 1

    Color attachments:
    -colorAttachmentCount = 1
    -pColorAttachments = &colorAttachmentInfo

    Depth attachment:
    -pDepthAttachment = &depthAttachmentInfo

    No traditional VkRenderPass or framebuffer object is supplied.

    This renderer uses Vulkan Dynamic Rendering.

    Step 10 — Begin dynamic rendering:

    -commandBuffer.beginRendering(renderingInfo)

    From this point until endRendering(), graphics draw commands operate using the supplied color and depth attachments.

    Step 11 — Bind graphics pipeline:

    -commandBuffer.bindPipeline(
        Graphics,
        graphicsPipeline
    )
    This activates the previously-created graphics pipeline for subsequent draw commands.

    Step 12 — Bind vertex buffer:

    -commandBuffer.bindVertexBuffers(
        0,
        vertexBuffer,
        {0}
    )
    First binding:
    -0

    Buffer:
    -vertexBuffer

    Buffer offset:
    -0 bytes

    This connects the GPU vertex buffer to vertex binding 0 expected by the graphics pipeline.

    Step 13 — Bind index buffer:

    -commandBuffer.bindIndexBuffer(
        indexBuffer,
        0,
        Uint32
    )

    Buffer:
    -indexBuffer

    Offset:
    -0

    Index type:
    -Uint32

    Therefore the renderer interprets indexBuffer contents as 32-bit unsigned integer indices.

    Step 14 — Bind descriptor set:

    -commandBuffer.bindDescriptorSets(
        Graphics,
        pipelineLayout,
        0,
        descriptorSets[frameIndex],
        nullptr
    )

    Bind point:
    -Graphics

    Pipeline layout:
    -pipelineLayout

    First descriptor-set number:
    -0

    Descriptor set:
    -descriptorSets[frameIndex]

    Dynamic offsets:
    -none

    Because the PipelineLayout contains one descriptor-set layout at set 0, this binds the current frame's descriptor set as:
        set = 0

    That descriptor set currently provides:
        binding 0 -> uniformBuffers[frameIndex]
        binding 1 -> textureImageView + textureSampler

    Step 15 — Set dynamic viewport:

    Call:
    -commandBuffer.setViewport(...)

    Current viewport:
    X: 0.0f

    Y: swapChainExtent.height

    Width: swapChainExtent.width

    Height: -negative swapChainExtent.height

    Depth range:
    -0.0f -> 1.0f

    Specifically:
        x = 0
        y = height
        width = +width
        height = -height

    The negative viewport height flips the viewport's Y direction.

    This is important because it handles the Vulkan/GLM coordinate-orientation difference in the viewport configuration.

    Therefore the current renderer does NOT also need the common projection-matrix adjustment:
        proj[1][1] *= -1
    in updateUniformBuffer().
    Performing both would flip Y twice.

    Step 16 — Set dynamic scissor:
    -commandBuffer.setScissor(...)

    Offset:
    -{0, 0}

    Extent:
    -swapChainExtent

    Therefore the full swapchain-sized render region is allowed to receive fragments.

    Step 17 — Issue indexed draw:

    -commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0)

    Parameters:

    indexCount:
    -indices.size()

    instanceCount: 1

    firstIndex: 0

    vertexOffset: 0

    firstInstance: 0

    Meaning:
    -Use every index currently stored in the model's index list.
    -Render one instance.
    -Begin reading indices at index 0.
    -Apply no additional base vertex offset.
    -Begin with instance 0.

    The draw therefore consumes:
        indexBuffer
            ↓ selects
        vertexBuffer
            ↓ processed through
        graphicsPipeline
            ↓ uses
        descriptorSets[frameIndex]
            ↓
        color/depth attachments

    Step 18 — End dynamic rendering:

    -commandBuffer.endRendering()

    This closes the dynamic-rendering scope begun by beginRendering().

    Step 19 — Transition swapchain image for presentation:

    Call transitionImageLayout() for:
    -swapChainImages[imageIndex]

    Transition:
    -ColorAttachmentOptimal
        ->
    -PresentSrcKHR

    Source access:
    -ColorAttachmentWrite

    Destination access:
    -none

    Source stage:
    -ColorAttachmentOutput

    Destination stage:
    -BottomOfPipe

    Aspect:
    -Color

    After the rendering and MSAA resolve operation, the acquired swapchain image is transitioned into the layout expected for presentation.

    Current swapchain-image flow:
        acquired image
            ↓
        Undefined
            ↓
        ColorAttachmentOptimal
            ↓ MSAA resolve destination
        rendered swapchain image
            ↓
        PresentSrcKHR
            ↓
        presentation system

    Step 20 — End command-buffer recording:

    -commandBuffer.end()

    The command buffer is now fully recorded.

    It has NOT yet executed.

    Execution occurs later when drawFrame() submits:

    -commandBuffers[frameIndex]

    to:
    -graphicsQueue

    Full recorded flow:
        commandBuffers[frameIndex]
            ↓ begin
        transition swapchain image
        transition MSAA color image
        transition depth image
            ↓
        configure color/depth attachments
            ↓
        beginRendering()
            ↓
        bind graphicsPipeline
        bind vertexBuffer
        bind indexBuffer
        bind descriptorSets[frameIndex]
        set viewport
        set scissor
            ↓
        drawIndexed()
            ↓
        endRendering()
            ↓
        transition swapchain image
        to PresentSrcKHR
            ↓
        commandBuffer.end()

    Important Consequences:
    -recordCommandBuffer() records into commandBuffers[frameIndex].
    -The acquired render target is selected independently through imageIndex.
    -The current frame's descriptor set is selected through frameIndex.
    -The swapchain image is used as the MSAA resolve destination rather than the primary multisampled attachment.
    -colorImage is the multisampled color attachment.
    -depthImage supplies depth testing.
    -Dynamic Rendering is used instead of a traditional RenderPass/Framebuffer pair.
    -The color attachment is cleared to opaque black.
    -Depth is cleared to 1.0.
    -The MSAA resolve mode is Average.
    -The color attachment uses loadOp Clear and storeOp Store.
    -The depth attachment uses loadOp Clear and storeOp DontCare.
    -The graphics pipeline is bound before drawing.
    -vertexBuffer is bound at binding 0 with offset 0.
    -indexBuffer is bound with Uint32 index interpretation.
    -descriptorSets[frameIndex] is bound as descriptor set 0.
    -No dynamic descriptor offsets are supplied.
    -Viewport and scissor are provided dynamically because they were declared dynamic pipeline states.
    -Negative viewport height performs the renderer's Y-axis flip.
    -drawIndexed() draws every current index once with one instance.
    -The swapchain image ends in PresentSrcKHR.
    -Recording commands does not execute them; queue submission happens later in drawFrame().

    // SOURCE RELATIONSHIP:
    // The negative viewport height in this function supplies the Y-axis
    // flip that is absent from updateUniformBuffer().
    //
    // Therefore Block 53's lack of:
    //
    //     ubo.proj[1][1] *= -1
    //
    // is consistent with the current viewport configuration.


    // FUTURE OPTIMIZATION:
    // colorAttachmentInfo currently uses:
    //
    //     storeOp = Store
    //
    // for the multisampled colorImage.
    //
    // Since the presentation result comes from the resolve image and the
    // multisampled attachment is not currently consumed afterward, this
    // may be a candidate for DontCare during a later performance/resource
    // audit.
    //
    // This is not required for correctness of the current documented flow.

    // FUTURE SYNCHRONIZATION AUDIT:
    // colorImage and depthImage are shared renderer resources rather than
    // being indexed per frame-in-flight.
    //
    // Their per-frame layout transitions and attachment-write dependencies
    // should remain part of any future synchronization/per-frame-resource
    // audit if the renderer is changed to permit more overlapping work.

    Source Mapping:
    recordCommandBuffer() complete per-frame graphics command recording path.


### Block 55 — Swapchain Recreation / `recreateSwapChain()`

    Purpose:

    Rebuild the renderer resources that depend directly on the swapchain when the existing swapchain becomes outdated or incompatible with the current window/surface state.

    My Configuration:

    Step 1 — Query current window pixel dimensions:

    Initialize:
    -width = 0
    -height = 0

    Call:
    -SDL_GetWindowSizeInPixels(
        window,
        &width,
        &height
    )

    If SDL_GetWindowSizeInPixels() fails:
    -throw std::runtime_error(...)

    The error message includes:
    -SDL_GetError()

    This means swapchain recreation begins by determining the current drawable pixel dimensions of the SDL window rather than assuming the previous swapchain dimensions are still valid.

    Step 2 — Handle zero-sized window:

    Check:
    -width == 0
        OR
    -height == 0

    If either dimension is zero:
    -return immediately

    Therefore no swapchain destruction or recreation occurs while the queried window pixel extent is zero.

    Current flow:
        query window pixel size
            ↓
        width or height == 0?
        ├── yes -> return
        └── no  -> continue recreation

    This is important for situations such as a minimized or temporarily zero-sized window where creating a normal swapchain extent would not be useful.

    Step 3 — Wait for the logical device to become idle:

    -device.waitIdle()

    This blocks until previously-submitted work on the logical device has completed before the swapchain-dependent resources are destroyed.

    Why:
    Resources such as:
    -swapchain images/views
    -MSAA color resources
    -depth resources

    may still be referenced by previously-submitted GPU work.

    Waiting for the device to become idle provides a simple, broad synchronization point before those resources are torn down.

    Important:
    -This is a device-wide idle wait.
    -It is intentionally much broader than waiting for a single frame fence.
    -Swapchain recreation is therefore currently a synchronous/stalling operation.

    Step 4 — Clean up old swapchain-dependent resources:

    Call:
    -cleanupSwapChain()

    This delegates destruction/reset of the old swapchain-dependent resources to the cleanupSwapChain() helper.

    cleanupSwapChain() currently removes:
    -depth image/view/memory resources
    -MSAA color image/view/memory resources
    -swapchain image views
    -stored swapchain image handles
    -the old swapchain

    The detailed destruction order and RAII behavior belong in the cleanupSwapChain() documentation rather than this recreation block.

    Important for this block:
    -cleanupSwapChain() does NOT clear or recreate synchronization objects.
    -renderFinishedSemaphore therefore survives this cleanup path unchanged.

    Step 5 — Create new swapchain:

    Call:
    -createSwapChain()

    This re-queries the current surface capabilities and creates a replacement swapchain using the engine's swapchain-selection logic.

    As part of createSwapChain():
    -swapChain is replaced
    -swapChainImages is repopulated
    -swapChainExtent may change
    -swapChainSurfaceFormat may potentially be re-selected

    Step 6 — Recreate swapchain image views:

    Call:
    -createImageViews()

    This creates one ImageView for each image in the newly-created swapchain.
    Relationship:
        new swapChain
            ↓
        new swapChainImages
            ↓
        new swapChainImageViews

    Step 7 — Recreate MSAA color resources:

    Call:
    -createColorResources()

    The MSAA color resource depends on swapchain properties such as:
    -extent
    -color format

    It is therefore rebuilt after the new swapchain exists.

    Step 8 — Recreate depth resources:

    Call:
    -createDepthResources()

    The depth image is also swapchain-sized and must therefore be recreated for the new extent.

    Current recreation sequence:
        SDL_GetWindowSizeInPixels()
            ↓
        zero extent?
            ├── yes -> return
            └── no
                ↓
        device.waitIdle()
            ↓
        cleanupSwapChain()
            ↓
        createSwapChain()
            ↓
        createImageViews()
            ↓
        createColorResources()
            ↓
        createDepthResources()

    Why:
    Several renderer resources are tied directly to properties of the swapchain.

    A window resize or an out-of-date/suboptimal presentation configuration can invalidate assumptions such as:
    -swapchain extent
    -swapchain images
    -swapchain image views
    -MSAA attachment dimensions
    -depth attachment dimensions

    Instead of attempting to modify those resources in place, the current engine destroys the old swapchain-dependent state and creates replacement resources matching the current surface/window configuration.

    Important Consequences:
    -Window pixel dimensions are checked before recreation begins.
    -SDL_GetWindowSizeInPixels() failure throws an exception.
    -A zero width or height causes recreation to return immediately.
    -device.waitIdle() ensures existing GPU work has completed before swapchain-dependent resources are destroyed.
    -The current recreation path stalls the entire logical device.
    -cleanupSwapChain() removes the previous swapchain, its image references/views, and the swapchain-sized color/depth resources.
    -A new swapchain is created.
    -swapChainImages is refreshed by createSwapChain().
    -Swapchain image views are recreated.
    -The MSAA color attachment is recreated.
    -The depth attachment is recreated.
    -swapChainExtent may change as part of createSwapChain().
    -The graphics pipeline is NOT recreated by this function.
    -Descriptor resources are NOT recreated by this function.
    -Command buffers are NOT recreated by this function.
    -presentCompleteSemaphore and inFlightFences are NOT recreated by this function.
    -renderFinishedSemaphore is NOT recreated or cleared by either recreateSwapChain() or cleanupSwapChain().

    // FUTURE SYNCHRONIZATION AUDIT:
    // createSyncObjects() creates:
    //
    //     renderFinishedSemaphore.size()
    //         =
    //     swapChainImages.size()
    //
    // recreateSwapChain() replaces swapChainImages, while cleanupSwapChain()
    // leaves renderFinishedSemaphore unchanged.
    //
    // Therefore, if the replacement swapchain contains a different number
    // of images, renderFinishedSemaphore may no longer match the new
    // swapChainImages.size().
    //
    // This dependency is now source-verified rather than uncertain.
    //
    // A future architecture pass should ensure that synchronization resources
    // indexed by imageIndex are recreated or resized when the swapchain image
    // count changes.

    // FUTURE PIPELINE-COMPATIBILITY AUDIT:
    // createSwapChain() may re-select swapChainSurfaceFormat.
    //
    // The graphics pipeline's Dynamic Rendering compatibility was created
    // using the swapchain color format and depth format.
    //
    // recreateSwapChain() does not recreate graphicsPipeline here.
    //
    // If a future swapchain recreation changes an attachment format,
    // pipeline compatibility should be re-evaluated.
    //
    // A simple extent-only change does not require pipeline recreation in
    // the current design because viewport and scissor are dynamic.

    Source Mapping:
    recreateSwapChain() swapchain-dependent resource rebuilding path.


### Block 56 — Application Main Loop / `mainLoop()`

    Purpose:
    Run the application's normal runtime loop by processing SDL events, tracking window state, requesting rendered frames while rendering is possible, and waiting for outstanding GPU work before leaving the loop.

    My Configuration:

    Main runtime loop:
    -while (!done)

    The application continues running until:
    -done = true

    Step 1 — Process pending SDL events:

    Create:
    -SDL_Event event

    Then:
    -while (SDL_PollEvent(&event))

    This inner loop processes all currently queued SDL events before normal frame rendering continues.

    Current event handling:

    SDL_EVENT_QUIT:
    -done = true
    This requests termination of the main application loop.

    SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    -framebufferResized = true
    This does NOT directly recreate the swapchain.

    Instead, it records that the drawable window size changed.

    drawFrame() later checks framebufferResized and handles swapchain recreation through the normal rendering path.

    SDL_EVENT_WINDOW_MINIMIZED:
    -minimized = true
    This prevents drawFrame() from being called while the window is minimized.

    SDL_EVENT_WINDOW_RESTORED:
    -minimized = false
    -framebufferResized = true

    Restoring the window therefore:
    -allows rendering to resume
    -marks the framebuffer/swapchain dimensions for reevaluation

    Current event relationship:
        SDL event
            ↓
        update engine state
            ↓
        rendering logic responds afterward

    Step 2 — Render when window is not minimized:

    Check:
    -if (!minimized)

    Then call:
    -drawFrame()

    Therefore:
        minimized == false
            ↓
        drawFrame()

        minimized == true
            ↓
        skip rendering

    The application still processes SDL events while minimized because the event loop remains active.

    Only normal frame rendering is skipped.

    Step 3 — Exit runtime loop:

    Once:
    -done == true
    the outer while loop terminates.

    Step 4 — Wait for outstanding GPU work:

    After leaving the loop:
    -device.waitIdle()

    This waits until outstanding work submitted to the logical device has completed before mainLoop() returns and shutdown/cleanup proceeds.

    Important:
    -device.waitIdle() is outside the main rendering loop.
    -The renderer does NOT make the entire device idle after every normal frame.
    -The wait occurs during shutdown after runtime rendering has finished.

    Runtime flow:
        while !done
            ↓
        poll all pending SDL events
            ↓
        update:
        ├── done
        ├── framebufferResized
        └── minimized
            ↓
        minimized?
        ├── yes -> skip drawFrame()
        └── no  -> drawFrame()
            ↓
        repeat
            ↓
        done == true
            ↓
        device.waitIdle()
            ↓
        mainLoop() returns

    Why:
    The application must continuously handle two broad responsibilities:

    1. CPU/window-system work:
    -process SDL events
    -detect shutdown
    -detect resize
    -detect minimize/restore

    2. Rendering work:
    -request another rendered frame when the window is in a renderable state

    The event loop keeps the application responsive while drawFrame() handles the actual Vulkan frame lifecycle.

    Important Consequences:
    -The outer loop continues until done becomes true.
    -SDL_PollEvent() is called repeatedly so all currently pending events are processed before the frame continues.
    -SDL_EVENT_QUIT sets done = true.
    -SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED only sets framebufferResized = true here; swapchain recreation occurs elsewhere.
    -SDL_EVENT_WINDOW_MINIMIZED sets minimized = true.
    -SDL_EVENT_WINDOW_RESTORED clears minimized and also marks framebufferResized = true.
    -Rendering is skipped while minimized.
    -SDL events continue to be processed while minimized.
    -drawFrame() drives normal continuous rendering whenever minimized is false.
    -device.waitIdle() occurs only after the main runtime loop exits.
    -Outstanding GPU work is therefore completed before later shutdown/destruction proceeds.

    // SOURCE BEHAVIOR NOTE:
    // SDL_EVENT_QUIT sets:
    //
    //     done = true
    //
    // inside the event-processing loop.
    //
    // The current outer-loop iteration does not immediately break or return.
    //
    // Therefore, if the window is not minimized, execution can still reach:
    //
    //     drawFrame()
    //
    // once during that same iteration before `while (!done)` is checked again.
    //
    // This is generally a minor control-flow detail rather than a fundamental
    // renderer issue, but it is the exact behavior of the current source.

    Source Mapping:
    mainLoop() application event-processing, runtime rendering, and shutdown-wait path.
   
### Block 57 — Per-Frame Rendering / `drawFrame()`

    Purpose:
    Coordinate everything required to safely render and request presentation of one frame.

    This includes:
    -waiting until the current frame-in-flight resources are safe to reuse
    -acquiring a swapchain image
    -resetting/re-recording the current command buffer
    -updating the current frame's uniform data
    -submitting rendering work
    -requesting presentation
    -handling swapchain changes
    -advancing to the next frame-in-flight slot

    My Configuration:

    Step 1 — Wait for current frame's previous GPU work:

    Call:
    -device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX)

    Fence:
    -inFlightFences[frameIndex]

    waitAll:
    -true

    timeout:
    -UINT64_MAX

    This causes the CPU to wait until the Fence associated with the current frame slot becomes signaled.

    Because each Fence tracks the previous graphics submission using that frame slot, a successful wait means resources such as:
    -commandBuffers[frameIndex]
    -uniformBuffers[frameIndex]
    -descriptorSets[frameIndex]
    can safely be reused for a new frame.

    Check result:

    If:
    -fenceResult != eSuccess

    Throw:
    -"Failed to wait for draw fence!"

    Step 2 — Acquire an available swapchain image:

    Call:

    -swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphore[frameIndex], nullptr)

    Timeout:
    -UINT64_MAX

    Signal semaphore:
    -presentCompleteSemaphore[frameIndex]

    Fence:
    -none

    Returns:
    -result
    -imageIndex

    imageIndex identifies which swapchain image Vulkan has made available for this frame.

    Therefore:
        frameIndex = reusable frame-resource slot
        imageIndex = acquired swapchain image

    These remain independent.

    If acquisition returns:
    -eErrorOutOfDateKHR

    Then:
    -recreateSwapChain()
    -return

    Important:
    The function returns before resetting the current frame Fence.

    This is significant because no new graphics submission will occur on this path.

    Leaving the Fence signaled prevents the frame slot from becoming stuck waiting on a Fence that no submission will ever signal.

    Step 3 — Reset current frame Fence:

    After successful continuation from image acquisition:
    -device.resetFences(*inFlightFences[frameIndex])

    The Fence was signaled because the previous submission completed.

    It is now reset to unsignaled so it can track completion of the new graphics submission.

    Step 4 — Reset current frame command buffer:

    -commandBuffers[frameIndex].reset()

    The command buffer's previous recorded commands are discarded so it can be recorded again for this frame.

    Step 5 — Record rendering commands:

    -recordCommandBuffer(imageIndex)

    This records into:
    -commandBuffers[frameIndex]

    while targeting:
    -swapChainImages[imageIndex]

    Relationship:
        frameIndex
            ↓
        commandBuffers[frameIndex]

        imageIndex
            ↓
        swapChainImages[imageIndex]

    Step 6 — Update current frame Uniform Buffer:

    -updateUniformBuffer(frameIndex)

    This writes the newest model/view/projection data into:
    -uniformBuffersMapped[frameIndex]

    Source-order note:

    recordCommandBuffer() occurs BEFORE updateUniformBuffer().

    This is valid in the current design because command recording binds the descriptor set and buffer references; it does not copy or consume the current UBO contents at recording time.
    The CPU updates the mapped Uniform Buffer before the recorded command buffer is submitted for GPU execution.

    Step 7 — Configure semaphore wait stage:

    Create:
    -vk::PipelineStageFlags waitDestinationStageMask{ColorAttachmentOutput}

    This is used as the destination stage mask for the acquisition semaphore wait.

    Meaning:
    The graphics submission must wait for:

    -presentCompleteSemaphore[frameIndex]

    before execution reaches the ColorAttachmentOutput stage where the acquired swapchain image becomes relevant to the current rendering work.

    Step 8 — Build graphics SubmitInfo:

    Wait semaphore count: 1

    Wait semaphore:
    -presentCompleteSemaphore[frameIndex]

    Wait destination stage:
    -ColorAttachmentOutput

    Command buffer count: 1

    Command buffer:
    -commandBuffers[frameIndex]

    Signal semaphore count: 1

    Signal semaphore:
    -renderFinishedSemaphore[imageIndex]

    Current submission relationship:
        presentCompleteSemaphore[frameIndex]
                    ↓ wait
        commandBuffers[frameIndex]
                    ↓ execute
        renderFinishedSemaphore[imageIndex]
                    ↓ signal

    Step 9 — Submit graphics work:

    Call:
    -graphicsQueue.submit(submitInfo, *inFlightFences[frameIndex])

    Queue:
    -graphicsQueue

    Submission completion Fence:
    -inFlightFences[frameIndex]

    This connects four synchronization/runtime resources:

        acquisition semaphore
            ↓
        graphics submission
            ↓
        render-finished semaphore

        graphics submission
            ↓
        inFlightFence

    The Semaphore chain coordinates GPU-side ordering.

    The Fence later allows the CPU to determine when this frame slot is safe to reuse.

    Step 10 — Build presentation information:

    Create:
    -vk::PresentInfoKHR presentInfo

    Wait semaphore count: 1

    Wait semaphore:
    -renderFinishedSemaphore[imageIndex]

    Presentation must therefore wait for rendering associated with that swapchain image to complete.

    Swapchain count: 1

    Swapchain:
    -swapChain

    Image index:
    -imageIndex

    Relationship:
        renderFinishedSemaphore[imageIndex]
                    ↓
        PresentInfoKHR
                    ↓
        swapChain imageIndex

    Step 11 — Request presentation:

    Call:
    -result = graphicsQueue.presentKHR(presentInfo)
    The same `result` variable previously used for image acquisition is reused here to store the presentation result.

    presentKHR requests presentation of:
    -swapChainImages[imageIndex]

    after:
    -renderFinishedSemaphore[imageIndex]
    has satisfied the presentation wait dependency.

    Step 12 — Handle swapchain changes:

    If presentation returns:
    -eErrorOutOfDateKHR
    OR:
    -eSuboptimalKHR
    OR:
    -framebufferResized == true

    Then:
    -framebufferResized = false
    -recreateSwapChain()

    This means swapchain recreation can be triggered by either:

    Vulkan presentation status:
    -OutOfDate
    -Suboptimal

    or application window state:
    -framebufferResized

    If presentation returns another result that is not:
    -eSuccess

    Then throw:
    -"Failed to present swapchain image!"

    Step 13 — Advance frame-in-flight slot:

    -frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT

    This rotates through the reusable frame-resource slots.

    Example with:

    -MAX_FRAMES_IN_FLIGHT = 2

    Sequence:
        0
        ↓
        1
        ↓
        0
        ↓
        1
        ...

    Important control-flow distinction:

    If acquireNextImage() returns eErrorOutOfDateKHR:
        recreateSwapChain()
            ↓
        return

    Therefore:
    -frameIndex is NOT advanced.


    If recreation occurs after presentation because of:
    -eErrorOutOfDateKHR
    -eSuboptimalKHR
    -framebufferResized

    then execution continues afterward to the frameIndex update.

    Therefore:
    -frameIndex IS advanced on that path.

    Why:
    drawFrame() coordinates four independently-progressing systems:

    -CPU execution
    -GPU execution
    -swapchain image availability
    -presentation

    The per-frame Fence answers:
        "Has the GPU finished the previous submission using this reusable frame slot?"

    acquireNextImage() answers:
        "Which swapchain image can this frame render into?"

    presentCompleteSemaphore[frameIndex] coordinates:
        swapchain image acquisition
            ->
        graphics use of that image

    renderFinishedSemaphore[imageIndex] coordinates:
        graphics completion
            ->
        presentation of that image

    The graphics submission ties these systems together without forcing device.waitIdle() every frame.

    Current high-level frame flow:
        wait for inFlightFences[frameIndex]
            ↓
        acquire swapchain image
            ↓
        imageIndex returned
            ↓
        reset frame Fence
            ↓
        reset commandBuffers[frameIndex]
            ↓
        recordCommandBuffer(imageIndex)
            ↓
        updateUniformBuffer(frameIndex)
            ↓
        submit graphics work
        ├── wait:
        │   presentCompleteSemaphore[frameIndex]
        │
        ├── execute:
        │   commandBuffers[frameIndex]
        │
        ├── signal:
        │   renderFinishedSemaphore[imageIndex]
        │
        └── completion Fence:
            inFlightFences[frameIndex]
            ↓
        present swapChain imageIndex
        waiting on renderFinishedSemaphore[imageIndex]
            ↓
        handle swapchain status
            ↓
        advance frameIndex

    Important Consequences:
    -The CPU waits only for the current frame slot's Fence rather than idling the entire device every frame.
    -Fence waiting uses UINT64_MAX, effectively requesting an indefinite wait for completion.
    -The Fence is not reset until after image acquisition successfully continues.
    -An acquire-time OutOfDate result recreates the swapchain and returns without advancing frameIndex.
    -commandBuffers[frameIndex] is reset and re-recorded every frame.
    -recordCommandBuffer() targets the independently-selected imageIndex.
    -The Uniform Buffer update happens before queue submission, so the GPU can consume the newly-written per-frame data during execution.
    -presentCompleteSemaphore is indexed by frameIndex.
    -commandBuffers and inFlightFences are indexed by frameIndex.
    -renderFinishedSemaphore is indexed by imageIndex.
    -The graphics submission waits at ColorAttachmentOutput on the acquisition semaphore.
    -One command buffer is submitted.
    -One render-finished Semaphore is signaled.
    -The frame Fence tracks completion of that graphics submission.
    -Presentation waits on renderFinishedSemaphore[imageIndex].
    -Exactly one swapchain and one imageIndex are supplied to PresentInfoKHR.
    -OutOfDate, Suboptimal, or framebufferResized can trigger swapchain recreation after presentation.
    -framebufferResized is reset to false before that recreation.
    -Unexpected presentation results throw an exception.
    -frameIndex advances modulo MAX_FRAMES_IN_FLIGHT after the normal/post-presentation path.

    // SOURCE BEHAVIOR NOTE:
    // recordCommandBuffer(imageIndex) occurs before:
    //
    //     updateUniformBuffer(frameIndex)
    //
    // This is compatible with the current design because command recording
    // stores the descriptor/buffer bindings, while the actual Uniform Buffer
    // contents are consumed later when the submitted commands execute.
    //
    // The UBO write occurs before graphicsQueue.submit().


    // RECREATION DEPENDENCY:
    // renderFinishedSemaphore is indexed by imageIndex.
    //
    // As established in Block 55, its size currently depends on the original
    // swapChainImages.size(), so swapchain recreation must eventually ensure
    // this array remains compatible if the swapchain image count changes.

    Source Mapping:
    drawFrame() complete per-frame synchronization, acquisition, recording, submission, presentation, and frame-slot advancement path.


# Shutdown

### Block 58 — `cleanup()`

    Purpose:
    Release the explicitly-managed SDL application resources after the main runtime loop has finished.

    My Configuration:
    Step 1 — Destroy SDL window:

    Call:
    -SDL_DestroyWindow(window)

    This releases the SDL window previously created during initWindow().

    The `window` pointer itself is not a Vulkan-Hpp RAII resource, so SDL is responsible for its destruction.

    Step 2 — Shut down SDL:
    Call:
    -SDL_Quit()

    This shuts down the SDL subsystems initialized for the application.

    Current cleanup flow:
        mainLoop()
            ↓
        device.waitIdle()
            ↓
        cleanup()
            ↓
        SDL_DestroyWindow(window)
            ↓
        SDL_Quit()

    Why:
    Most Vulkan resources in the current renderer are managed through Vulkan-Hpp RAII wrapper objects.

    cleanup() therefore does not contain a large sequence of explicit Vulkan destroy calls.

    The remaining SDL resources are not Vulkan RAII objects and must be explicitly released through SDL.

    Important Consequences:
    -The SDL window is explicitly destroyed.
    -SDL is explicitly shut down.
    -cleanup() does NOT call device.waitIdle().
    -The device-wide wait currently occurs at the end of mainLoop() before cleanup() is reached.
    -cleanup() does NOT call cleanupSwapChain().
    -cleanup() does NOT explicitly destroy Vulkan Images, Buffers, ImageViews, Samplers, Pipelines, Descriptor objects, synchronization objects, the swapchain, device, surface, or instance.
    -Those Vulkan resources are managed separately through the current Vulkan-Hpp RAII ownership/lifetime system.
    -cleanup() is therefore primarily the explicit SDL shutdown stage rather than the complete Vulkan resource-destruction implementation.

    // RAII LIFETIME NOTE:
    // VulkanEngine's Vulkan resources are stored using Vulkan-Hpp RAII
    // wrappers throughout the current renderer.
    //
    // Their destruction is therefore governed by RAII object lifetime rather
    // than by explicit destruction calls inside cleanup().
    //
    // This is separate from cleanupSwapChain(), which is used during runtime
    // swapchain recreation to release selected swapchain-dependent resources
    // before new replacements are created.

    Source Mapping:
    cleanup() explicit SDL application shutdown path.
