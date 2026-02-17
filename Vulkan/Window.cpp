#include "Window.h"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#include <ktx.h>
#include <ktxvulkan.h>

#define LOG(msg) \
	do { \
		std::cout << "[LOG]" \
		<< msg \
		<< "| FILE: " << __FILE__\
		<< "| Function:" << __func__ \
		<< "| Line:" << __LINE__ \
		<< std::endl; \
	} while (0);




int Window::Init(uint32_t x, uint32_t y) {
	_version = 0;
	vkEnumerateInstanceVersion(&_version);

	std::cout << "Vulkan version: "
		<< VK_VERSION_MAJOR(_version) << "."
		<< VK_VERSION_MINOR(_version) << "."
		<< VK_VERSION_PATCH(_version) << std::endl;

	// init glfw
	if (glfwInit() != GLFW_TRUE) {
		LOG("Failed to initialize GLFW.\n");
		return EXIT_FAILURE;
	}

	uint32_t instanceExtensionsCount = { 0 };
	const char** instanceExtensions = glfwGetRequiredInstanceExtensions(&instanceExtensionsCount);

	VkApplicationInfo appInfo{
	.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	.pApplicationName = "Vox",
	.apiVersion = VK_API_VERSION_1_3
	};

	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
	VkInstanceCreateInfo instanceCI{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 1,
		.ppEnabledLayerNames = layers,
		.enabledExtensionCount = instanceExtensionsCount,
		.ppEnabledExtensionNames = instanceExtensions,
	};
	
	chk(vkCreateInstance(
		&instanceCI,
		nullptr,
		&_instance
	));

	// COMPUTER FIND THE 4070 TI SUPER
	uint32_t deviceCount = 0;
	chk(vkEnumeratePhysicalDevices(
		_instance,
		&deviceCount,
		nullptr
	));
	
	_devices.resize(deviceCount);

	chk(vkEnumeratePhysicalDevices(
		_instance,
		&deviceCount,
		_devices.data()
	));

	if (deviceCount == 0) exit(-1);

	VkPhysicalDeviceProperties2 deviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	vkGetPhysicalDeviceProperties2(_devices[0], &deviceProperties);
	std::cout << "Selected device: " << deviceProperties.properties.deviceName << "\n";



	// Work is queued instead of submitted directly to device
	// differnt types of queue
	// e.g. graphics, compute, transfer, video


	// get the list of queue families.
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(_devices[0], &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(_devices[0], &queueFamilyCount, queueFamilies.data());
		
	// find the graphics queue 
	uint32_t queueFamily = 0;
	for (size_t i = 0; i < queueFamilies.size(); i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamily = i;
			break;
		}
	}


	if (glfwGetPhysicalDevicePresentationSupport(_instance, _devices[0], queueFamily) != GLFW_TRUE)
	{
		LOG("Device does not support presentation");
		exit(-1);
	}


	const float qfPriorities =  1.f;

	VkDeviceQueueCreateInfo queueCI{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = queueFamily,
		.queueCount = 1,
		.pQueuePriorities = &qfPriorities
	};

	const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceVulkan12Features enabledVk12Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.descriptorIndexing = true,
		.shaderSampledImageArrayNonUniformIndexing = true,
		.descriptorBindingVariableDescriptorCount = true,
		.runtimeDescriptorArray = true,
		.bufferDeviceAddress = true
	};
	const VkPhysicalDeviceVulkan13Features enabledVk13Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &enabledVk12Features,
		.synchronization2 = true,
		.dynamicRendering = true,
	};
	const VkPhysicalDeviceFeatures enabledVk10Features{
		.samplerAnisotropy = VK_TRUE
	};


	VkDeviceCreateInfo deviceCI{
	.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	.pNext = &enabledVk13Features,
	.queueCreateInfoCount = 1,
	.pQueueCreateInfos = &queueCI,
	.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
	.ppEnabledExtensionNames = deviceExtensions.data(),
	.pEnabledFeatures = &enabledVk10Features
	};
	chk(vkCreateDevice(_devices[0], &deviceCI, nullptr, &_device));

	// 4 Q <- fuck you (pun - fuck you)
	vkGetDeviceQueue(_device, queueFamily, 0, &_queue);

	// set up vulkan memory allocator
	// structtastic

	VmaVulkanFunctions vkFunctions{
	.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
	.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
		.vkCreateImage = vkCreateImage
	};

	VmaAllocatorCreateInfo allocatorCI{
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = _devices[0],
		.device = _device,
		.pVulkanFunctions = &vkFunctions,
		.instance = _instance
	};

	chk(vmaCreateAllocator(&allocatorCI, &_allocator));

	// create window 
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	_window = glfwCreateWindow(x, y, "Vox", nullptr, nullptr);

	if (_window == nullptr) {
		LOG("Failed to create GLFW window.\n");
		glfwTerminate();
		return EXIT_FAILURE;
	}

	int w, h;
	glfwGetWindowSize(_window, &w, &h);
	width = static_cast<float>(w);
	height = static_cast<float>(h);

	glfwSetScrollCallback(_window, scrollCallback);
	glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);

	chk(glfwCreateWindowSurface(_instance, _window, nullptr, &_surface));

	chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_devices[0], _surface, &_surfaceCaps));


	_swapchainCI = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = _surface,
		.minImageCount = _surfaceCaps.minImageCount,
		.imageFormat = _imageFormat,
		.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
		.imageExtent{.width = _surfaceCaps.currentExtent.width, .height = _surfaceCaps.currentExtent.height },
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR
	};
	chk(vkCreateSwapchainKHR(_device, &_swapchainCI, nullptr, &_swapchain));


	uint32_t _imageCount{ 0 };
	chk(vkGetSwapchainImagesKHR(_device, _swapchain, &_imageCount, nullptr));
	_swapchainImages.resize(_imageCount);
	chk(vkGetSwapchainImagesKHR(_device, _swapchain, &_imageCount, _swapchainImages.data()));
	_swapchainImageViews.resize(_imageCount);

	for (auto i = 0; i < _imageCount; i++) {
		VkImageViewCreateInfo viewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = _swapchainImages[i], .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = _imageFormat, .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
		chk(vkCreateImageView(_device, &viewCI, nullptr, &_swapchainImageViews[i]));
	}

	std::vector<VkFormat> depthFormatList{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	for (VkFormat& format : depthFormatList) {
		VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
		vkGetPhysicalDeviceFormatProperties2(_devices[0], format, &formatProperties);
		if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			_depthFormat = format;
			break;
		}
	}

	_depthImageCI = {
	.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	.imageType = VK_IMAGE_TYPE_2D,
	.format = _depthFormat,
	.extent{.width = x, .height = y, .depth = 1 },
	.mipLevels = 1,
	.arrayLayers = 1,
	.samples = VK_SAMPLE_COUNT_1_BIT,
	.tiling = VK_IMAGE_TILING_OPTIMAL,
	.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VmaAllocationCreateInfo allocCI{
		.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO
	};

	chk(vmaCreateImage(_allocator, &_depthImageCI, &allocCI, &_depthImage, &_depthImageAllocation, nullptr));

	VkImageViewCreateInfo depthViewCI{
	.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	.image = _depthImage,
	.viewType = VK_IMAGE_VIEW_TYPE_2D,
	.format = _depthFormat,
	.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
	};

	chk(vkCreateImageView(_device, &depthViewCI, nullptr, &_depthImageView));

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	if (tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, "assets/suzanne.obj") == false)
	{
		LOG("Failed to load model");
		exit(-1);
	}

	_indexCount = { shapes[0].mesh.indices.size() };
	std::vector<Vertex> vertices{};
	std::vector<uint16_t> indices{};
	// Load vertex and index data
	for (auto& index : shapes[0].mesh.indices) {
		Vertex v{
			.pos = { attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2] },
			.normal = { attrib.normals[index.normal_index * 3], -attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2] },
			.uv = { attrib.texcoords[index.texcoord_index * 2], 1.0 - attrib.texcoords[index.texcoord_index * 2 + 1] }
		};
		vertices.push_back(v);
		indices.push_back(indices.size());
	}

	_vBufSize = { sizeof(Vertex) * vertices.size() };
	VkDeviceSize iBufSize{ sizeof(uint16_t) * indices.size() };
	VkBufferCreateInfo bufferCI{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = _vBufSize + iBufSize,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
	};

	VmaAllocationCreateInfo bufferAllocCI{
	.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
	.usage = VMA_MEMORY_USAGE_AUTO
	};
	chk(vmaCreateBuffer(_allocator, &bufferCI, &bufferAllocCI, &_vBuffer, &_vBufferAllocation, nullptr));

	void* bufferPtr{ nullptr };
	chk(vmaMapMemory(_allocator, _vBufferAllocation, &bufferPtr));
	memcpy(bufferPtr, vertices.data(), _vBufSize);
	memcpy(((char*)bufferPtr) + _vBufSize, indices.data(), iBufSize);
	vmaUnmapMemory(_allocator, _vBufferAllocation);

	for (auto i = 0; i < maxFramesInFlight; i++) {
		VkBufferCreateInfo uBufferCI{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(ShaderData),
			.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		};
		VmaAllocationCreateInfo uBufferAllocCI{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO
		};
		chk(vmaCreateBuffer(_allocator, &uBufferCI, &uBufferAllocCI, &_shaderDataBuffers[i].buffer, &_shaderDataBuffers[i].allocation, nullptr));
		chk(vmaMapMemory(_allocator, _shaderDataBuffers[i].allocation, &_shaderDataBuffers[i].mapped));

		VkBufferDeviceAddressInfo uBufferBdaInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = _shaderDataBuffers[i].buffer
		};
		_shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(_device, &uBufferBdaInfo);
	}


	VkSemaphoreCreateInfo semaphoreCI{
	.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkFenceCreateInfo fenceCI{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (auto i = 0; i < maxFramesInFlight; i++) {
		chk(vkCreateFence(_device, &fenceCI, nullptr, &_fences[i]));
		chk(vkCreateSemaphore(_device, &semaphoreCI, nullptr, &_presentSemaphores[i]));
	}

	_renderSemaphores.resize(_swapchainImages.size());
	for (auto& semaphore : _renderSemaphores) {
		chk(vkCreateSemaphore(_device, &semaphoreCI, nullptr, &semaphore));
	}


	VkCommandPoolCreateInfo commandPoolCI{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queueFamily
	};

	chk(vkCreateCommandPool(_device, &commandPoolCI, nullptr, &_commandPool));

	VkCommandBufferAllocateInfo cbAllocCI{
	.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	.commandPool = _commandPool,
	.commandBufferCount = maxFramesInFlight
	};
	chk(vkAllocateCommandBuffers(_device, &cbAllocCI, _commandBuffers.data()));


	for (auto i = 0; i < _textures.size(); i++) {
		ktxTexture* ktxTexture{ nullptr };
		std::string filename = "assets/suzanne" + std::to_string(i) + ".ktx";
		ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

		VkImageCreateInfo texImgCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = ktxTexture_GetVkFormat(ktxTexture),
			.extent = {.width = ktxTexture->baseWidth, .height = ktxTexture->baseHeight, .depth = 1 },
			.mipLevels = ktxTexture->numLevels,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};

		VmaAllocationCreateInfo texImageAllocCI{ .usage = VMA_MEMORY_USAGE_AUTO };
		chk(vmaCreateImage(_allocator, &texImgCI, &texImageAllocCI, &_textures[i].image, &_textures[i].allocation, nullptr));


		VkImageViewCreateInfo texVewCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = _textures[i].image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = texImgCI.format,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
		};

		chk(vkCreateImageView(_device, &texVewCI, nullptr, &_textures[i].view));

		VkBuffer imgSrcBuffer{};
		VmaAllocation imgSrcAllocation{};
		VkBufferCreateInfo imgSrcBufferCI{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = (uint32_t)ktxTexture->dataSize,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		};

		VmaAllocationCreateInfo imgSrcAllocCI{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO
		};

		chk(vmaCreateBuffer(_allocator, &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer, &imgSrcAllocation, nullptr));


		void* imgSrcBufferPtr{ nullptr };
		chk(vmaMapMemory(_allocator, imgSrcAllocation, &imgSrcBufferPtr));
		memcpy(imgSrcBufferPtr, ktxTexture->pData, ktxTexture->dataSize);


		VkFenceCreateInfo fenceOneTimeCI{
	.		sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
		};
		VkFence fenceOneTime{};

		chk(vkCreateFence(_device, &fenceOneTimeCI, nullptr, &fenceOneTime));

		VkCommandBuffer cbOneTime{};
		VkCommandBufferAllocateInfo cbOneTimeAI{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = _commandPool,
			.commandBufferCount = 1
		};

		chk(vkAllocateCommandBuffers(_device, &cbOneTimeAI, &cbOneTime));


		VkCommandBufferBeginInfo cbOneTimeBI{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		chk(vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI));

		VkImageMemoryBarrier2 barrierTexImage{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
			.srcAccessMask = VK_ACCESS_2_NONE,
			.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.image = _textures[i].image,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
		};

		VkDependencyInfo barrierTexInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &barrierTexImage
		};

		vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);

		std::vector<VkBufferImageCopy> copyRegions{};
		for (auto j = 0; j < ktxTexture->numLevels; j++) {
			ktx_size_t mipOffset{ 0 };
			KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
			copyRegions.push_back({
				.bufferOffset = mipOffset,
				.imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)j, .layerCount = 1},
				.imageExtent{.width = ktxTexture->baseWidth >> j, .height = ktxTexture->baseHeight >> j, .depth = 1 },
				});
		}

		vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, _textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

		VkImageMemoryBarrier2 barrierTexRead{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			.image = _textures[i].image,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
		};
		barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;

		vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
		chk(vkEndCommandBuffer(cbOneTime));

		VkSubmitInfo oneTimeSI{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &cbOneTime
		};

		chk(vkQueueSubmit(_queue, 1, &oneTimeSI, fenceOneTime));
		chk(vkWaitForFences(_device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX));


		VkSamplerCreateInfo samplerCI{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = 8.0f, // 8 is a widely supported value for max anisotropy
			.maxLod = (float)ktxTexture->numLevels,
		};

		chk(vkCreateSampler(_device, &samplerCI, nullptr, &_textures[i].sampler));

		ktxTexture_Destroy(ktxTexture);
		_textureDescriptors.push_back({
			.sampler = _textures[i].sampler,
			.imageView = _textures[i].view,
			.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
			});
	}

	VkDescriptorBindingFlags descVariableFlag{ VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT };
	VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = 1,
		.pBindingFlags = &descVariableFlag
	};

	VkDescriptorSetLayoutBinding descLayoutBindingTex{
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = static_cast<uint32_t>(_textures.size()),
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};

	VkDescriptorSetLayoutCreateInfo descLayoutTexCI{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &descBindingFlags,
		.bindingCount = 1,
		.pBindings = &descLayoutBindingTex
	};

	chk(vkCreateDescriptorSetLayout(_device, &descLayoutTexCI, nullptr, &_descriptorSetLayoutTex));


	VkDescriptorPoolSize poolSize{
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = static_cast<uint32_t>(_textures.size())
	};

	VkDescriptorPoolCreateInfo descPoolCI{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.poolSizeCount = 1,
		.pPoolSizes = &poolSize
	};

	chk(vkCreateDescriptorPool(_device, &descPoolCI, nullptr, &_descriptorPool));


	uint32_t variableDescCount{ static_cast<uint32_t>(_textures.size()) };
	VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
		.descriptorSetCount = 1,
		.pDescriptorCounts = &variableDescCount
	};

	VkDescriptorSetAllocateInfo texDescSetAlloc{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = &variableDescCountAI,
		.descriptorPool = _descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &_descriptorSetLayoutTex
	};

	chk(vkAllocateDescriptorSets(_device, &texDescSetAlloc, &_descriptorSetTex));


	VkWriteDescriptorSet writeDescSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = _descriptorSetTex,
		.dstBinding = 0,
		.descriptorCount = static_cast<uint32_t>(_textureDescriptors.size()),
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = _textureDescriptors.data()
	};

	vkUpdateDescriptorSets(_device, 1, &writeDescSet, 0, nullptr);

	slang::createGlobalSession(_slangGlobalSession.writeRef());

	auto slangTargets{ std::to_array<slang::TargetDesc>({ {
		.format{SLANG_SPIRV},
		.profile{_slangGlobalSession->findProfile("spirv_1_4")}
	} }) };

	auto slangOptions{ std::to_array<slang::CompilerOptionEntry>({ {
		slang::CompilerOptionName::EmitSpirvDirectly,
		{slang::CompilerOptionValueKind::Int, 1}
	} }) };

	slang::SessionDesc slangSessionDesc{
		.targets{slangTargets.data()},
		.targetCount{SlangInt(slangTargets.size())},
		.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
		.compilerOptionEntries{slangOptions.data()},
		.compilerOptionEntryCount{uint32_t(slangOptions.size())}
	};

	Slang::ComPtr<slang::ISession> slangSession;
	_slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());

	Slang::ComPtr<slang::IModule> slangModule{
		slangSession->loadModuleFromSource("triangle", "assets/shader.slang", nullptr, nullptr)
	};

	Slang::ComPtr<ISlangBlob> spirv;
	slangModule->getTargetCode(0, spirv.writeRef());

	VkShaderModuleCreateInfo shaderModuleCI{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = spirv->getBufferSize(),
		.pCode = (uint32_t*)spirv->getBufferPointer()
	};
	VkShaderModule shaderModule{};

	chk(vkCreateShaderModule(_device, &shaderModuleCI, nullptr, &shaderModule));


	VkPushConstantRange pushConstantRange{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.size = sizeof(VkDeviceAddress)
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &_descriptorSetLayoutTex,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange
	};

	chk(vkCreatePipelineLayout(_device, &pipelineLayoutCI, nullptr, &_pipelineLayout));

	VkVertexInputBindingDescription vertexBinding{
	 .binding = 0,
	 .stride = sizeof(Vertex),
	 .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	std::vector<VkVertexInputAttributeDescription> vertexAttributes{
		{.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT },
		{.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal) },
		{.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv) },
	};

	VkPipelineVertexInputStateCreateInfo vertexInputState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexBinding,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
		.pVertexAttributeDescriptions = vertexAttributes.data(),
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	};

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
		{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		  .stage = VK_SHADER_STAGE_VERTEX_BIT,
		  .module = shaderModule, .pName = "main"},
		{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		  .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		  .module = shaderModule, .pName = "main" }
	};

	VkPipelineViewportStateCreateInfo viewportState{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	.viewportCount = 1,
	.scissorCount = 1
	};

	std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamicStates.data()
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilState{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	.depthTestEnable = VK_TRUE,
	.depthWriteEnable = VK_TRUE,
	.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
	};

	VkPipelineRenderingCreateInfo renderingCI{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
	.colorAttachmentCount = 1,
	.pColorAttachmentFormats = &_imageFormat,
	.depthAttachmentFormat = _depthFormat
	};

	VkPipelineColorBlendAttachmentState blendAttachment{
	.colorWriteMask = 0xF
	};
	VkPipelineColorBlendStateCreateInfo colorBlendState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &blendAttachment
	};
	VkPipelineRasterizationStateCreateInfo rasterizationState{
		 .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		 .lineWidth = 1.0f
	};
	VkPipelineMultisampleStateCreateInfo multisampleState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
	};


	VkGraphicsPipelineCreateInfo pipelineCI{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &renderingCI,
		.stageCount = 2,
		.pStages = shaderStages.data(),
		.pVertexInputState = &vertexInputState,
		.pInputAssemblyState = &inputAssemblyState,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizationState,
		.pMultisampleState = &multisampleState,
		.pDepthStencilState = &depthStencilState,
		.pColorBlendState = &colorBlendState,
		.pDynamicState = &dynamicState,
		.layout = _pipelineLayout
	};

	chk(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &_pipeline));
}



void Window::Update(){
	// Wait on fence
	chk(vkWaitForFences(_device, 1, &_fences[_frameIndex], true, UINT64_MAX));
	chk(vkResetFences(_device, 1, &_fences[_frameIndex]));
	// Acquire next image
	chkSwapchain(vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _presentSemaphores[_frameIndex], VK_NULL_HANDLE, &_imageIndex));
	// Update shader data
	shaderData.projection = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 32.0f);
	shaderData.view = glm::translate(glm::mat4(1.0f), camPos);
	for (auto i = 0; i < 3; i++) {
		auto instancePos = glm::vec3((float)(i - 1) * 3.0f, 0.0f, 0.0f);
		shaderData.model[i] = glm::translate(glm::mat4(1.0f), instancePos) * glm::mat4_cast(glm::quat(objectRotations[i]));
	}
	// Record command buffer
	auto cb = _commandBuffers[_frameIndex];

	chk(vkResetCommandBuffer(cb, 0));


	VkCommandBufferBeginInfo cbBI{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	chk(vkBeginCommandBuffer(cb, &cbBI));


	std::array<VkImageMemoryBarrier2, 2> outputBarriers{
		VkImageMemoryBarrier2{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.image = _swapchainImages[_imageIndex],
			.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
		},
		VkImageMemoryBarrier2{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.image = _depthImage,
			.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 }
		}
	};

	VkDependencyInfo barrierDependencyInfo{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 2,
		.pImageMemoryBarriers = outputBarriers.data()
	};

	vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);

	VkRenderingAttachmentInfo colorAttachmentInfo{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = _swapchainImageViews[_imageIndex],
		.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue{.color{ 0.0f, 0.0f, 0.2f, 1.0f }}
	};

	VkRenderingAttachmentInfo depthAttachmentInfo{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = _depthImageView,
		.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.clearValue = {.depthStencil = {1.0f,  0}}
	};

	VkRenderingInfo renderingInfo{
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea{.extent{.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height) }},
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentInfo,
		.pDepthAttachment = &depthAttachmentInfo
	};

	vkCmdBeginRendering(cb, &renderingInfo);

	VkViewport vp{
		.width = width,
		.height = height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	vkCmdSetViewport(cb, 0, 1, &vp);

	VkRect2D scissor{ .extent{.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height) } };

	vkCmdSetScissor(cb, 0, 1, &scissor);

	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);


	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSetTex, 0, nullptr);

	VkDeviceSize vOffset{ 0 };
	vkCmdBindVertexBuffers(cb, 0, 1, &_vBuffer, &vOffset);

	vkCmdBindIndexBuffer(cb, _vBuffer, _vBufSize, VK_INDEX_TYPE_UINT16);


	vkCmdPushConstants(cb, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &_shaderDataBuffers[_frameIndex].deviceAddress);


	vkCmdDrawIndexed(cb, _indexCount, 3, 0, 0, 0);


	vkCmdEndRendering(cb);


	VkImageMemoryBarrier2 barrierPresent{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = 0,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.image = _swapchainImages[_imageIndex],
		.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
	};

	VkDependencyInfo barrierPresentDependencyInfo{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrierPresent
	};

	vkCmdPipelineBarrier2(cb, &barrierPresentDependencyInfo);

	vkEndCommandBuffer(cb);

	// Submit command buffer
	VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &_presentSemaphores[_frameIndex],
		.pWaitDstStageMask = &waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &cb,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &_renderSemaphores[_imageIndex],
	};

	chk(vkQueueSubmit(_queue, 1, &submitInfo, _fences[_frameIndex]));

	_frameIndex = (_frameIndex + 1) % maxFramesInFlight;

	// Present image
	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &_renderSemaphores[_imageIndex],
		.swapchainCount = 1,
		.pSwapchains = &_swapchain,
		.pImageIndices = &_imageIndex
	};
	chkSwapchain(vkQueuePresentKHR(_queue, &presentInfo));
	// Poll events

	float currentTime = static_cast<float>(glfwGetTime());
	float elapsedTime = currentTime - _lastTime;
	_lastTime = currentTime;

	glfwPollEvents();

	double mouseX, mouseY;
	glfwGetCursorPos(_window, &mouseX, &mouseY);

	static double lastMouseX = mouseX;
	static double lastMouseY = mouseY;
	int leftButtonState = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT);

	if (leftButtonState == GLFW_PRESS) {
		double dx = mouseX - lastMouseX;
		double dy = mouseY - lastMouseY;
		objectRotations[shaderData.selected].x -= static_cast<float>(dy) * elapsedTime;
		objectRotations[shaderData.selected].y += static_cast<float>(dx) * elapsedTime;
	}

	lastMouseX = mouseX;
	lastMouseY = mouseY;

	camPos.z += scrollY * elapsedTime * 10.0f;
	scrollY = 0.0f;

	if (glfwGetKey(_window, GLFW_KEY_KP_ADD) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
		shaderData.selected = (shaderData.selected < 2) ? shaderData.selected + 1 : 0;
	}
	if (glfwGetKey(_window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_MINUS) == GLFW_PRESS) {
		shaderData.selected = (shaderData.selected > 0) ? shaderData.selected - 1 : 2;
	}

	if (_updateSwapchain) {
		_updateSwapchain = false;
		vkDeviceWaitIdle(_device);
		chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_devices[0], _surface, &_surfaceCaps));
		_swapchainCI.oldSwapchain = _swapchain;
		_swapchainCI.imageExtent = { .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height) };
		chk(vkCreateSwapchainKHR(_device, &_swapchainCI, nullptr, &_swapchain));
		for (auto i = 0; i < _imageCount; i++) {
			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
		}
		chk(vkGetSwapchainImagesKHR(_device, _swapchain, &_imageCount, nullptr));
		_swapchainImages.resize(_imageCount);
		chk(vkGetSwapchainImagesKHR(_device, _swapchain, &_imageCount, _swapchainImages.data()));
		_swapchainImageViews.resize(_imageCount);
		for (auto i = 0; i < _imageCount; i++) {
			VkImageViewCreateInfo viewCI{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = _swapchainImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = _imageFormat,
				.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}
			};
			chk(vkCreateImageView(_device, &viewCI, nullptr, &_swapchainImageViews[i]));
		}
		vkDestroySwapchainKHR(_device, _swapchainCI.oldSwapchain, nullptr);
		vmaDestroyImage(_allocator, _depthImage, _depthImageAllocation);
		vkDestroyImageView(_device, _depthImageView, nullptr);
		_depthImageCI.extent = { .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height), .depth = 1 };
		VmaAllocationCreateInfo allocCI{
			.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO
		};
		chk(vmaCreateImage(_allocator, &_depthImageCI, &allocCI, &_depthImage, &_depthImageAllocation, nullptr));
		VkImageViewCreateInfo viewCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = _depthImage,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = _depthFormat,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
		};
		chk(vkCreateImageView(_device, &viewCI, nullptr, &_depthImageView));
	}
}


int Window::Destroy(){
	glfwDestroyWindow(_window);
	glfwTerminate();
	return EXIT_SUCCESS;
}

 bool Window::ShouldClose() {
	 return glfwWindowShouldClose(_window);
 }

 inline void Window::chk(VkResult result)
 {
	if (result)
	{
		LOG(VkResultToString(result));
		exit(result);
	}
 }

 inline void Window::chkSwapchain(VkResult result)
 {
	 if (result < VK_SUCCESS) {
		 if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			 _updateSwapchain = true;
			 return;
		 }
		 std::cerr << "Vulkan call returned an error (" << result << ")\n";
		 exit(result);
	 }
 }

 void Window::scrollCallback(GLFWwindow* w, double xoffset, double yoffset)
 {
	 scrollY += static_cast<float>(yoffset);
 }

 void Window::framebufferResizeCallback(GLFWwindow* w, int width, int height)
 {
	 _updateSwapchain = true;
	 Window::width = static_cast<float>(width);
	 Window::height = static_cast<float>(height);
 }

