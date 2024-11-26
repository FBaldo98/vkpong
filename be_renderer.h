#pragma once

#include "be_window.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>
#include <optional>

namespace be
{
	namespace renderer {
		struct QueueFamilyIndices
		{
			::std::optional<uint32_t> graphicsFamily;
			::std::optional<uint32_t> presentFamily;

			bool isComplete()
			{
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};

		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities;
			::std::vector<VkSurfaceFormatKHR> formats;
			::std::vector<VkPresentModeKHR> presentModes;
		};

		class BeRenderer
		{
		public:
			BeRenderer(BeWindow* window) : window(window) {}
			~BeRenderer();

			bool init();

		private:
			void setupDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT& createInfo);
			void getVkPhysicalDevice();
			int rateDeviceSuitability(VkPhysicalDevice device);

			void createLogicDevice();
			void createSurface();
			void createSwapChain();
			void createImageViews();
			void createGraphicsPipeline();

			VkShaderModule createShaderModule(const ::std::vector<char>& code);

			VkPresentModeKHR chooseSwapPresentMode(const ::std::vector<VkPresentModeKHR>& availablePresentModes);
			VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
			VkSurfaceFormatKHR chooseSwapSurfaceFormat(const ::std::vector<VkSurfaceFormatKHR>& availableFormats);

			QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
			bool checkDeviceExtensionSupport(VkPhysicalDevice device);
			SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

			void terminateVk();

			::std::vector<const char*> getRequiredExtensions();
			bool checkValidationLayerSupport();

		private:
			BeWindow* window = nullptr;

			VkInstance vkInstance = VK_NULL_HANDLE;
			VkDebugUtilsMessengerEXT vkDebugMessenger = VK_NULL_HANDLE;

			VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
			VkDevice vkDevice = VK_NULL_HANDLE;

			VkQueue graphicsQueue = VK_NULL_HANDLE;
			VkQueue presentQueue = VK_NULL_HANDLE;
			VkSwapchainKHR swapChain = VK_NULL_HANDLE;

			VkSurfaceKHR vkSurface = VK_NULL_HANDLE;

			VkFormat swapChainImageFormat;
			VkExtent2D swapChainExtent;

			::std::vector<VkImage> swapChainImages;
			::std::vector<VkImageView> swapChainImageViews;

			const ::std::vector<const char*> validationLayers = {
				"VK_LAYER_KHRONOS_validation"
			};

			const ::std::vector<const char*> deviceExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};

			static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageType,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData);

#ifdef _DEBUG
			const bool enableValidationLayers = true;
#else
			const bool enableValidationLayers = false;
#endif
		};
	}
}