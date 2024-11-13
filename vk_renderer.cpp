#include "vk_renderer.h"

#include <Windows.h>

bool vk_init(VkContext* vkContext, void* window)
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Pong";
    appInfo.pEngineName = "PongEngine";

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    VK_CHECK(vkCreateInstance(&instanceInfo, 0, &vkContext->instance));

    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = (HWND)window;
    surfaceCreateInfo.hinstance = GetModuleHandle(0);

    VK_CHECK(vkCreateWin32SurfaceKHR(vkContext->instance, &surfaceCreateInfo, 0, &vkContext->surface));

    return true;
}