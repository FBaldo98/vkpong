#pragma once

#include <iostream>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define VK_CHECK(result)                  \
    if (result != VK_SUCCESS)             \
    {                                     \
        std::cout << "Vulkan Error: " << result << std::endl; \
        __debugbreak();                   \
        return false;                     \
    }

struct VkContext
{
    VkInstance instance;
    VkSurfaceKHR surface;
};

bool vk_init(VkContext* vkContext, void* window);