#include "be_renderer.h"

#include <iostream>
#include <stdexcept>
#include <map>
#include <set>

#include <cstdint>
#include <limits>
#include <algorithm>

#include "utils.h";

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

			createRenderPass();

			createGraphicsPipeline();

			createFramebuffers();

			createCommandTool();

			createCommandBuffer();

			createSyncObjects();

			return true;
		}

		void BeRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
				throw std::runtime_error("Failed to begin recording command buffer");

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = vkRenderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;

			VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
				
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);

			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(swapChainExtent.width);
			viewport.height = static_cast<float>(swapChainExtent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = swapChainExtent;
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			vkCmdDraw(commandBuffer, 3, 1, 0, 0);	

			vkCmdEndRenderPass(commandBuffer);

			if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
				throw std::runtime_error("Failed to end command buffer");
		}

		void BeRenderer::drawFrame()
		{
			vkWaitForFences(vkDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
			vkResetFences(vkDevice, 1, &inFlightFence);

			uint32_t imageIndex;
			vkAcquireNextImageKHR(vkDevice, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

			vkResetCommandBuffer(commandBuffer, imageIndex);
			recordCommandBuffer(commandBuffer, imageIndex);

			VkSubmitInfo submitInfo = {};;
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
				throw std::runtime_error("failed to submit draw command buffer!");

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = { swapChain };
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;

			vkQueuePresentKHR(presentQueue, &presentInfo);
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

			std::vector<VkDynamicState> dynamicStates = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			VkPipelineDynamicStateCreateInfo dynamicState = {};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicState.pDynamicStates = dynamicStates.data();

			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 0;
			vertexInputInfo.pVertexBindingDescriptions = nullptr;
			vertexInputInfo.vertexAttributeDescriptionCount = 0;
			vertexInputInfo.pVertexAttributeDescriptions = nullptr;

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			// We use dynamic viewport and scissor state
			//VkViewport viewport = {};
			//viewport.x = 0.0f;
			//viewport.y = 0.0f;
			//viewport.width = (float)swapChainExtent.width;
			//viewport.height = (float)swapChainExtent.height;
			//viewport.minDepth = 0.0f;
			//viewport.maxDepth = 1.0f;

			//VkRect2D scissor = {};
			//scissor.offset = { 0, 0 };
			//scissor.extent = swapChainExtent;

			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterizer = {};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizer.depthBiasEnable = VK_FALSE;
			
			VkPipelineMultisampleStateCreateInfo multisampling = {};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;

			VkPipelineColorBlendStateCreateInfo colorBlending = {};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			
			if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS)
				throw std::runtime_error("Failed to create pipeline layout");

			VkGraphicsPipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = nullptr;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = &dynamicState;
			pipelineInfo.layout = vkPipelineLayout;
			pipelineInfo.renderPass = vkRenderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.basePipelineIndex = -1;

			if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkGraphicsPipeline) != VK_SUCCESS)
				throw std::runtime_error("failed to create graphics pipeline");

			vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
			vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
		}

		void BeRenderer::createRenderPass()
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = swapChainImageFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			if (vkCreateRenderPass(vkDevice, &renderPassInfo, nullptr, &vkRenderPass) != VK_SUCCESS)
				throw std::runtime_error("failed to create render pass");
		}

		void BeRenderer::createFramebuffers()
		{
			swapChainFramebuffers.resize(swapChainImageViews.size());

			for (size_t i = 0; i < swapChainImageViews.size(); i++)
			{
				VkImageView attachments[] = {
					swapChainImageViews[i]
				};

				VkFramebufferCreateInfo framebufferInfo = {};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = vkRenderPass;
				framebufferInfo.attachmentCount = 1;
				framebufferInfo.pAttachments = attachments;
				framebufferInfo.width = swapChainExtent.width;
				framebufferInfo.height = swapChainExtent.height;
				framebufferInfo.layers = 1;

				if (vkCreateFramebuffer(vkDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
					throw std::runtime_error("Failed to create framebuffer!");
			}
		}

		void BeRenderer::createCommandTool()
		{
			QueueFamilyIndices queueFamilyIndices = findQueueFamilies(vkPhysicalDevice);

			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
			 
			if (vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
				throw std::runtime_error("failed to create command pool");
		}

		void BeRenderer::createCommandBuffer()
		{
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;

			if (vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer) != VK_SUCCESS)
				throw std::runtime_error("failed to allocate command buffer");

		}

		void BeRenderer::createSyncObjects()
		{
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
				vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
				vkCreateFence(vkDevice, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create semaphore");
			}
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
			vkDestroySemaphore(vkDevice, imageAvailableSemaphore, nullptr);
			vkDestroySemaphore(vkDevice, renderFinishedSemaphore, nullptr);
			vkDestroyFence(vkDevice, inFlightFence, nullptr);

			vkDestroyCommandPool(vkDevice, commandPool, nullptr);

			for (auto framebuffer : swapChainFramebuffers)
				vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);

			vkDestroyPipeline(vkDevice, vkGraphicsPipeline, nullptr);
			vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
			vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);

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