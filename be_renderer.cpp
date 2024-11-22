#include "be_renderer.h"

#include <iostream>
#include <stdexcept>
#include <map>
#include <set>

#include <cstdint>
#include <limits>
#include <algorithm>

#include "utils.h"

namespace be {
	namespace renderer {

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

			createSwapChain();

			createImageViews();

			createGraphicsPipeline();

			return true;
		}

		void BeRenderer::setupDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT& createInfo)
		{
			if (!enableValidationLayers) return;

			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr)
			{
				if (func(vkInstance, &createInfo, nullptr, &vkDebugMessenger) == VK_SUCCESS)
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
				score += 1000;
			else if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
				score += 500;
			else if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
				score += 250;

			score += deviceProps.limits.maxImageDimension2D;

			if (!deviceFeatures.geometryShader)
				return 0;

			auto indices = findQueueFamilies(device);
			if (!indices.isComplete())
				return 0;

			auto extensionsSupported = checkDeviceExtensionSupport(device);
			if (!extensionsSupported)
				return 0;

			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
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
			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();

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

		void BeRenderer::createSwapChain()
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vkPhysicalDevice);

			VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
			VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
			VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

			swapChainExtent = extent;
			swapChainImageFormat = surfaceFormat.format;

			uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
			if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
				imageCount = swapChainSupport.capabilities.maxImageCount;

			VkSwapchainCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = vkSurface;
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = extent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			QueueFamilyIndices indices = findQueueFamilies(vkPhysicalDevice);
			uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

			if (indices.graphicsFamily != indices.presentFamily)
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0;
				createInfo.pQueueFamilyIndices = nullptr;
			}

			createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = VK_NULL_HANDLE;

			if (vkCreateSwapchainKHR(vkDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
				throw std::runtime_error("Could not create swapchain");

			vkGetSwapchainImagesKHR(vkDevice, swapChain, &imageCount, nullptr);
			swapChainImages.resize(imageCount);
			vkGetSwapchainImagesKHR(vkDevice, swapChain, &imageCount, swapChainImages.data());
		}

		void BeRenderer::createImageViews()
		{
			swapChainImageViews.resize(swapChainImages.size());

			for (size_t i = 0; i < swapChainImages.size(); i++)
			{
				VkImageViewCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				createInfo.image = swapChainImages[i];
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				createInfo.format = swapChainImageFormat;
				
				createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

				createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.levelCount = 1;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.layerCount = 1;

				if (vkCreateImageView(vkDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
					throw std::runtime_error("Failed to create image view");
			}
		}

		void BeRenderer::createGraphicsPipeline()
		{
			auto vertShaderCode = readFile("shaders/vert.spv");
			auto fragShaderCode = readFile("shaders/frag.spv");

			VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
			VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

			VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = vertShaderModule;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = fragShaderModule;
			fragShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

			vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
			vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
		}

		VkShaderModule BeRenderer::createShaderModule(const::std::vector<char>& code)
		{
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

			VkShaderModule shaderModule;
			if (vkCreateShaderModule(vkDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
				throw std::runtime_error("Could not create shader module");

			return shaderModule;
		}

		VkPresentModeKHR BeRenderer::chooseSwapPresentMode(const ::std::vector<VkPresentModeKHR>& availablePresentModes)
		{
			for (const auto& apMode : availablePresentModes)
			{
				if (apMode == VK_PRESENT_MODE_MAILBOX_KHR)
					return apMode;
			}
			
			return VK_PRESENT_MODE_FIFO_KHR;
		}

		VkExtent2D BeRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
		{
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
				return capabilities.currentExtent;
			else
			{
				RECT rr;
				GetClientRect(window->window, &rr);

				VkExtent2D actualExtent = {
					static_cast<uint32_t>(rr.right - rr.left),
					static_cast<uint32_t>(rr.bottom - rr.top)
				};

				actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

				return actualExtent;
			}

			return VkExtent2D();
		}

		VkSurfaceFormatKHR BeRenderer::chooseSwapSurfaceFormat(const ::std::vector<VkSurfaceFormatKHR>& availableFormats)
		{
			for (const auto& aFormat : availableFormats)
			{
				if (aFormat.format == VK_FORMAT_B8G8R8A8_SRGB && aFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					return aFormat;
			}

			return availableFormats[0];
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

				if (indices.isComplete())
					break;

				i++;
			}

			return indices;
		}

		bool BeRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
		{
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

			std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

			for (const auto& extension : availableExtensions)
			{
				requiredExtensions.erase(extension.extensionName);
			}

			return requiredExtensions.empty();
		}

		SwapChainSupportDetails BeRenderer::querySwapChainSupport(VkPhysicalDevice device)
		{
			SwapChainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vkSurface, &details.capabilities);

			uint32_t formatsCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkSurface, &formatsCount, nullptr);

			if (formatsCount != 0)
			{
				details.formats.resize(formatsCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkSurface, &formatsCount, details.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkSurface, &presentModeCount, nullptr);

			if (presentModeCount != 0)
			{
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkSurface, &presentModeCount, details.presentModes.data());
			}

			return details;
		}

		void BeRenderer::terminateVk()
		{
			for (auto iv : swapChainImageViews)
				vkDestroyImageView(vkDevice, iv, nullptr);

			vkDestroySwapchainKHR(vkDevice, swapChain, nullptr);
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
}