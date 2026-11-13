cmake_minimum_required(VERSION 3.20)

project(WizEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


# ---------------------------------------------------------
# Vulkan SDK
# ---------------------------------------------------------

# WizEngine currently uses components provided by the
# locally installed LunarG Vulkan SDK, including Slang,
# GLM, and VMA.
#
# The Vulkan SDK installer sets the VULKAN_SDK
# environment variable automatically.

if(NOT DEFINED ENV{VULKAN_SDK} OR "$ENV{VULKAN_SDK}" STREQUAL "")
    message(FATAL_ERROR
        "VULKAN_SDK environment variable is not set. "
        "Install or configure the LunarG Vulkan SDK."
    )
endif()


# ---------------------------------------------------------
# tinygltf
# ---------------------------------------------------------

set_source_files_properties(
    external/tinygltf/tiny_gltf_v3.c
    PROPERTIES
        COMPILE_DEFINITIONS TINYGLTF3_ENABLE_FS
)


# ---------------------------------------------------------
# Slang shaders
# ---------------------------------------------------------

# Graphics shader
set(GRAPHICS_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/graphics.slang"
)

set(GRAPHICS_SHADER_OUTPUT
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/graphics.spv"
)


# Compute shader
set(COMPUTE_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/compute.slang"
)

set(COMPUTE_SHADER_OUTPUT
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/compute.spv"
)


# Particle shader
set(PARTICLE_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/particle.slang"
)

set(PARTICLE_SHADER_OUTPUT
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/particle.spv"
)


# ---------------------------------------------------------
# System-installed dependencies
# ---------------------------------------------------------

find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG REQUIRED)


# Find the Slang compiler supplied by the Vulkan SDK.
#
# find_program() also searches normal system PATH locations,
# while the HINTS explicitly point it toward the Vulkan SDK.

find_program(
    SLANGC_EXECUTABLE

    NAMES
        slangc
        slangc.exe

    HINTS
        "$ENV{VULKAN_SDK}/Bin"
        "$ENV{VULKAN_SDK}/bin"

    REQUIRED
)


# ---------------------------------------------------------
# Compile graphics shader
# ---------------------------------------------------------

add_custom_command(
    OUTPUT "${GRAPHICS_SHADER_OUTPUT}"

    COMMAND "${SLANGC_EXECUTABLE}"
        "${GRAPHICS_SHADER_SOURCE}"
        -target spirv
        -profile spirv_1_4
        -emit-spirv-directly
        -fvk-use-entrypoint-name
        -entry vertMain
        -entry fragMain
        -o "${GRAPHICS_SHADER_OUTPUT}"

    DEPENDS
        "${GRAPHICS_SHADER_SOURCE}"

    COMMENT
        "Compiling graphics.slang to graphics.spv"

    VERBATIM
)


# ---------------------------------------------------------
# Compile compute shader
# ---------------------------------------------------------

add_custom_command(
    OUTPUT "${COMPUTE_SHADER_OUTPUT}"

    COMMAND "${SLANGC_EXECUTABLE}"
        "${COMPUTE_SHADER_SOURCE}"
        -target spirv
        -profile spirv_1_4
        -emit-spirv-directly
        -fvk-use-entrypoint-name
        -entry compMain
        -o "${COMPUTE_SHADER_OUTPUT}"

    DEPENDS
        "${COMPUTE_SHADER_SOURCE}"

    COMMENT
        "Compiling compute.slang to compute.spv"

    VERBATIM
)


# ---------------------------------------------------------
# Compile particle shader
# ---------------------------------------------------------

add_custom_command(
    OUTPUT "${PARTICLE_SHADER_OUTPUT}"

    COMMAND "${SLANGC_EXECUTABLE}"
        "${PARTICLE_SHADER_SOURCE}"
        -target spirv
        -profile spirv_1_4
        -emit-spirv-directly
        -fvk-use-entrypoint-name
        -entry particleVertMain
        -entry particleFragMain
        -o "${PARTICLE_SHADER_OUTPUT}"

    DEPENDS
        "${PARTICLE_SHADER_SOURCE}"

    COMMENT
        "Compiling particle.slang to particle.spv"

    VERBATIM
)


# Compile all shaders when this target is built.
add_custom_target(
    compile_shaders

    DEPENDS
        "${GRAPHICS_SHADER_OUTPUT}"
        "${COMPUTE_SHADER_OUTPUT}"
        "${PARTICLE_SHADER_OUTPUT}"
)


# ---------------------------------------------------------
# WizEngine executable
# ---------------------------------------------------------

add_executable(
    WizEngine

    src/main.cpp
    src/vk_engine.cpp
    src/vk_device.cpp
    src/stb_image.cpp
    src/logger.cpp

    external/tinyobj/tiny_obj_loader.cpp
    external/tinygltf/tiny_gltf_v3.c
)

add_dependencies(
    WizEngine
    compile_shaders
)


# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------

target_include_directories(
    WizEngine

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include

        # Vendored dependencies
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
        ${CMAKE_CURRENT_SOURCE_DIR}/external/tinyobj
        ${CMAKE_CURRENT_SOURCE_DIR}/external/tinygltf

        # Vulkan SDK
        #
        # Provides Vulkan headers and installed SDK components
        # such as GLM.
        "$ENV{VULKAN_SDK}/Include"

        # Allows:
        # #include <vk_mem_alloc.h>
        "$ENV{VULKAN_SDK}/Include/vma"
)


# ---------------------------------------------------------
# KTX
# ---------------------------------------------------------

set(BUILD_SHARED_LIBS OFF)

add_subdirectory(
    external/KTX-Software/lib
    ktx
)


# ---------------------------------------------------------
# Libraries
# ---------------------------------------------------------

target_link_libraries(
    WizEngine

    PRIVATE
        Vulkan::Vulkan
        SDL3::SDL3
        ktx
)