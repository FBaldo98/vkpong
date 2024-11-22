#include "be_renderer.h"

#include <iostream>
#include <stdexcept>
#include <map>
#include <set>

namespace be {


	BeRenderer::~BeRenderer()
	{
		terminateVk();
	}

	bool BeRenderer::init()
	{
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("Validation layers not supported!!");
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "VkPong";
		appInfo.pEngineName = "be_engine";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		auto requiredExtensions = getRequiredExtensions();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;
		debugCreateInfo.pUserData = nullptr;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		if (enableValidationLayers)
		{
			createInfo.ppEnabledLayerNames = validationLayers.data();
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS)
		{
			throw std::runtime_error("Error creating instance");
		}

		setupDebugMessenger(debugCreateInfo);

		createSurface();

		getVkPhysicalDevice();

		createLogicDevice();

		return true;
	}

	void BeRenderer::setupDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		if (!enableValidationLayers) return;

		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			if(func(vkInstance, &createInfo, nullptr, &vkDebugMessenger) == VK_SUCCESS)
				return;
		}
		throw std::runtime_error("Failed to setup debug messenger");
	}

	void BeRenderer::getVkPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

		if (deviceCount == 0)
			throw std::runtime_error("Failed to find GPUs with Vulkan support");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());


		std::multimap<int, VkPhysicalDevice> candidates;
		for (const auto& device : devices)
		{
			int score = rateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}

		if (candidates.rbegin()->first > 0)
			vkPhysicalDevice = candidates.rbegin()->second;
		else
			throw std::runtime_error("Failed to find suitable GPU");

	}

	int BeRenderer::rateDeviceSuitability(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProps;
		VkPhysicalDeviceFeatures deviceFeatures;

		vkGetPhysicalDeviceProperties(device, &deviceProps);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		int score = 0;

		if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 1000;
		}
		else if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			score += 500;
		}

		score += deviceProps.limits.maxImageDimension2D;

		if (!deviceFeatures.geometryShader)
			return 0;

		auto indices = findQueueFamilies(device);
		if (!indices.isComplete())
			return 0;

		return score;
	}

	void BeRenderer::createLogicDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(vkPhysicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };


		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = 0;

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(vkPhysicalDevice, &createInfo, nullptr, &vkDevice) != VK_SUCCESS)
			throw std::runtime_error("Failed to create logical device");

		vkGetDeviceQueue(vkDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(vkDevice, indices.presentFamily.value(), 0, &presentQueue);
	}

	void BeRenderer::createSurface()
	{
		VkWin32SurfaceCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = window->window;
		createInfo.hinstance = GetModuleHandle(nullptr);

		if (vkCreateWin32SurfaceKHR(vkInstance, &createInfo, nullptr, &vkSurface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create win32 surface");
	}

	QueueFamilyIndices BeRenderer::findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vkSurface, &presentSupport);
			if (presentSupport) indices.presentFamily = i;
		}

		return indices;
	}

	void BeRenderer::terminateVk()
	{

		vkDestroyDevice(vkDevice, nullptr);

		vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);

		if (enableValidationLayers)
		{
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr)
				func(vkInstance, vkDebugMessenger, nullptr);
		}

		vkDestroyInstance(vkInstance, nullptr);
	}

	std::vector<const char*> BeRenderer::getRequiredExtensions()
	{
		uint32_t extensionCount = 0;
		if (!vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) == VK_SUCCESS)
		{
			throw std::runtime_error("Error enumerating extensions");
		}

		std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
		if (!vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data()) == VK_SUCCESS)
		{
			throw std::runtime_error("Error enumerating extensions");
		}

		std::vector<const char*> requiredExtensions;
		for (auto& ext : supportedExtensions)
		{
			char* ex = (char*)malloc(sizeof(char) * 256);
			strcpy_s(ex, sizeof(char) * 256, ext.extensionName);
			requiredExtensions.push_back(ex);
		}

		return requiredExtensions;
	}

	bool BeRenderer::checkValidationLayerSupport()
	{
		uint32_t validationLayerCount = 0;
		vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);

		std::vector<VkLayerProperties> supportedLayers(validationLayerCount);
		vkEnumerateInstanceLayerProperties(&validationLayerCount, supportedLayers.data());

		for (auto layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& supported : supportedLayers)
			{
				if (strcmp(supported.layerName, layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound) return false;
		}

		return true;
	}
	VKAPI_ATTR VkBool32 VKAPI_CALL BeRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

			return VK_FALSE;
	}
}