#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <windows.h>
#include <vector>
#include <glm/glm.hpp>
#include "commonvulkan.h"

static const char* EXE_NAME = "VulkanTriangle";
static const uint32_t VERTEX_BUFFER_BIND_ID = 0;
#define VALIDATION_LAYERS true
#define DEBUGGING true


//vertex data store don ram
struct Vertex
{
	float pos[3];
	float col[3];
};

struct CameraPosition
{
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;

};

struct Camera
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDescriptorBufferInfo desc;

	CameraPosition position;
};

//vertex data stored on the gpu ram
struct VertexBuffer
{
	//vertex placement data
	std::vector<Vertex> vPos;
	VkBuffer vBuffer;
	VkDeviceMemory vMemory;
	VkPipelineVertexInputStateCreateInfo viInfo;
	std::vector<VkVertexInputBindingDescription> vBindingDesc;
	std::vector<VkVertexInputAttributeDescription> vBindingAttr;

	//vertex index data
	std::vector<uint32_t> iPos;
	int iCount;
	VkBuffer iBuffer;
	VkDeviceMemory iMemory;

};



//main struct, pretty much holds everything
struct MainMemory
{
	HWND consoleHandle;
	HWND windowHandle;
	HINSTANCE exeHandle;

	uint32_t clientWidth, clientHeight;

	bool running;

	VkInstance vkInstance;

	VkSurfaceKHR surface;
	VkColorSpaceKHR surfaceColorSpace;
	VkFormat surfaceColorFormat;
	uint32_t surfaceImageCount;
	std::vector<VkImage> surfaceImages;
	std::vector<SwapChainBuffer> surfaceBuffers;

	VkPhysicalDevice physicalDevice;

	uint32_t renderingQueueFamilyIndex;
	VkFormat supportedDepthFormat;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceMemoryProperties memoryProperties;

	std::vector<const char*> instanceLayerList;
	std::vector<const char*> instanceExtList;
	std::vector<const char*> deviceLayerList;
	std::vector<const char*> deviceExtList;
	VkDebugReportCallbackEXT debugReport;

	VkDevice logicalDevice;
	VkQueue queue;
	VkCommandPool cmdPool;

	VkSemaphore presentComplete;
	VkSemaphore renderComplete;

	//TODO better name?
	VkSubmitInfo submitInfo;

	VkCommandBuffer setupCommandBuffer;
	std::vector<VkCommandBuffer> drawCmdBuffers;
	VkCommandBuffer prePresentCmdBuffer;
	VkCommandBuffer	postPresentCmdBuffer;

	DepthStencil depthStencil;

	VkRenderPass renderPass;
	VkPipelineCache pipelineCache;
	std::vector<VkFramebuffer> frameBuffers;

	VkCommandBuffer textureCmdBuffer;
	VertexBuffer vertexBuffer;

	Camera camera;
	VkDescriptorSetLayout descriptorSetlayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

};