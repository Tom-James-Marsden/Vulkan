#pragma once
#include <vector>
#include <array>
#include <vma/vk_mem_alloc.h>

#include "Includes.h"
#include "Helper.h"
#include "tiny_obj_loader.h"
#include "slang/slang.h"
#include "slang/slang-com-ptr.h"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct ShaderData {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model[3];
	glm::vec4 lightPos{ 0.0f, -10.0f, 10.0f, 0.0f };
	uint32_t selected{ 1 };
};

struct ShaderDataBuffer {
	VmaAllocation allocation{ VK_NULL_HANDLE };
	VkBuffer buffer{ VK_NULL_HANDLE };
	VkDeviceAddress deviceAddress{};
	void* mapped{ nullptr };
};

struct Texture {
	VmaAllocation allocation{ VK_NULL_HANDLE };
	VkImage image{ VK_NULL_HANDLE };
	VkImageView view{ VK_NULL_HANDLE };
	VkSampler sampler{ VK_NULL_HANDLE };
};

constexpr uint32_t maxFramesInFlight{ 2 };

class Window
{
public:
	int			Init(uint32_t x, uint32_t y);
	int			Destroy();


	void		Update();
	bool		ShouldClose();

	inline static float width = 0.f;
	inline static float height = 0.f;

	inline static float scrollY = 0.f;



private:
	inline void chk(VkResult result);
	inline void chkSwapchain(VkResult result);
	static void scrollCallback(GLFWwindow* w, double xoffset, double yoffset);
	static void framebufferResizeCallback(GLFWwindow* w, int width, int height);

	uint32_t	_version = 0;
	GLFWwindow* _window;
	VkInstance _instance;
	VkDevice _device;
	VkQueue _queue;
	VmaAllocator _allocator;
	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapchain;

	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	VkImage _depthImage;
	VmaAllocation _depthImageAllocation;
	VkImageView _depthImageView;

	VkBuffer _vBuffer;
	VmaAllocation _vBufferAllocation;

	std::array<ShaderDataBuffer, maxFramesInFlight> _shaderDataBuffers;
	std::array<VkCommandBuffer, maxFramesInFlight> _commandBuffers;
	std::array<VkFence, maxFramesInFlight> _fences;
	std::array<VkSemaphore, maxFramesInFlight> _presentSemaphores;

	std::vector<VkSemaphore> _renderSemaphores;

	VkCommandPool _commandPool;

	std::array<Texture, 3> _textures;

	std::vector<VkDescriptorImageInfo> _textureDescriptors;

	VkDescriptorSetLayout _descriptorSetLayoutTex;

	VkDescriptorPool _descriptorPool;

	VkDescriptorSet _descriptorSetTex;

	Slang::ComPtr<slang::IGlobalSession> _slangGlobalSession;

	VkPipelineLayout _pipelineLayout;
	VkPipeline _pipeline;

	uint32_t _frameIndex{ 0 };
	uint32_t _imageIndex{ 0 };

	inline static bool _updateSwapchain = false;

	ShaderData shaderData{};

	glm::vec3 camPos{ 0.0f, 0.0f, -6.0f };

	glm::vec3 objectRotations[3]{};

	VkDeviceSize _vBufSize{ 0 };

	VkDeviceSize _indexCount{ 0 };

	uint32_t _imageCount{ 0 };
	const VkFormat _imageFormat{ VK_FORMAT_B8G8R8A8_SRGB };
	VkFormat _depthFormat{ VK_FORMAT_UNDEFINED };
	VkImageCreateInfo _depthImageCI;
	std::vector<VkPhysicalDevice> _devices;
	VkSurfaceCapabilitiesKHR _surfaceCaps{};
	VkSwapchainCreateInfoKHR _swapchainCI{};

	float _lastTime{ 0.0f };
};

