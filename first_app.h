#pragma once

#include "be_window.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>

namespace be 
{
	
	class FirstApp
	{
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		FirstApp() {
			initWindow(window, WIDTH, HEIGHT, "Hello World");

			initVk();
		}

		void run();

	private:
		bool initVk();
		bool getVkPhysicalDevice();
		void terminateVk();

		std::vector<const char*> getRequiredExtensions();
		bool checkValidationLayerSupport();

	private:
		BeWindow window = {};

		VkInstance vkInstance;

		const std::vector<const char*> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

#ifdef _DEBUG
		const bool enableValidationLayers = true;
#else
		const bool enableValidationLayers = false;
#endif
	};

}