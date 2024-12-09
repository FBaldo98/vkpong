#pragma once

#include "be_window.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <vector>
#include <optional>
#include <array>

namespace be
{
	namespace renderer {

		const int MAX_FRAMES_IN_FLIGHT = 2;

		struct Vertex {
			glm::vec2 pos;
			glm::vec3 color;

			static VkVertexInputBindingDescription getBindingDescription()
			{
				VkVertexInputBindingDescription description = {};

				description.binding = 0;
				description.stride = sizeof(Vertex);
				description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				return description;
			}

			static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() 
			{
				std::array<VkVertexInputAttributeDescription, 2> descriptions = {};

				descriptions[0].binding = 0;
				descriptions[0].location = 0;
				descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
				descriptions[0].offset = offsetof(Vertex, pos);

				descriptions[1].binding = 0;
				descriptions[1].location = 1;
				descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
				descriptions[1].offset = offsetof(Vertex, color);

				return descriptions;
			}
		};

		const ::std::vector<Vertex> vertices = {
			{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
		};

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
			void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
			void drawFrame();

		private:
			void setupDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT& createInfo);
			void getVkPhysicalDevice();
			int rateDeviceSuitability(VkPhysicalDevice device);

			void createLogicDevice();
			void createSurface();
			void createSwapChain();
			void createImageViews();
			void createGraphicsPipeline();
			void createRenderPass();
			void createFramebuffers();
			void createCommandTool();
			void createCommandBuffer();
			void createSyncObjects();
			void createVertexBuffer();

			void cleanupSwapChain();
			void recreateSwapChain();

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

			VkRenderPass vkRenderPass = VK_NULL_HANDLE;
			VkPipeline vkGraphicsPipeline = VK_NULL_HANDLE;
			VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
			VkCommandPool commandPool;

			VkBuffer vertexBuffer;

			uint32_t currentFrame = 0;
			::std::vector<VkCommandBuffer> commandBuffers;

			::std::vector<VkSemaphore> imageAvailableSemaphores;
			::std::vector<VkSemaphore> renderFinishedSemaphores;
			::std::vector<VkFence> inFlightFences;

			::std::vector<VkImage> swapChainImages;
			::std::vector<VkImageView> swapChainImageViews;
			::std::vector<VkFramebuffer> swapChainFramebuffers;

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