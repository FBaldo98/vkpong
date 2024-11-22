#include "first_app.h"

#include <stdexcept>

namespace be {

	void FirstApp::run()
	{
		while (::g_running) {
			window.handleMessages();
		}
	}

	bool FirstApp::initVk()
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

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		createInfo.enabledExtensionCount = requiredExtensions.size();
		if (enableValidationLayers)
		{
			createInfo.ppEnabledLayerNames = validationLayers.data();
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS)
		{
			throw std::runtime_error("Error creating instance");
		}

		return true;
	}

	bool FirstApp::getVkPhysicalDevice()
	{


		return true;
	}

	void FirstApp::terminateVk()
	{
		vkDestroyInstance(vkInstance, nullptr);
	}

	std::vector<const char*> FirstApp::getRequiredExtensions()
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
			strcpy_s(ex, sizeof(char)*256, ext.extensionName);
			requiredExtensions.push_back(ex);
		}

		return requiredExtensions;
	}

	bool FirstApp::checkValidationLayerSupport()
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

}