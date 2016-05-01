#pragma once


#include <vulkan/vulkan.h>
#include <vector>
#include "surface.h""



//TODO dont really know
//TODO rename?
struct DepthStencil
{
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
};

//temporary buffers and memory used to move the vertex data to the gpu
struct StagingBuffer
{
	VkDeviceMemory memory;
	VkBuffer buffer;
};

struct StagingBuffers
{
	StagingBuffer vertices;
	StagingBuffer indices;
};


struct PhysDeviceInfo
{
	VkPhysicalDevice physicalDevice;
	uint32_t renderingQueueFamilyIndex;
	VkFormat supportedDepthFormat;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceMemoryProperties memoryProperties;
};

struct DeviceInfo
{
	VkDevice device;
	VkQueue queue;
	VkCommandPool cmdPool;

	VkSemaphore presentComplete;
	VkSemaphore renderComplete;

	VkCommandBuffer setupCmdBuffer;
	VkCommandBuffer prePresentCmdBuffer;
	VkCommandBuffer	postPresentCmdBuffer;

	std::vector<VkCommandBuffer> drawCmdBuffers;
	std::vector<VkFramebuffer> frameBuffers;
};

struct PipelineInfo
{
	VkRenderPass renderPass;
	VkPipelineCache pipelineCache;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkDescriptorPool descriptorPool;
	VkPipeline pipeline;
};

struct DebugInfo
{
	std::vector<const char*> instanceLayerList;
	std::vector<const char*> instanceExtList;
	std::vector<const char*> deviceLayerList;
	std::vector<const char*> deviceExtList;
	VkDebugReportCallbackEXT debugReport;
};


VkDescriptorSetLayout NewDescriptorSetLayout(VkDevice logicalDevice, VkDescriptorType type, VkShaderStageFlags flags);

VkPipelineLayout NewPipelineLayout(VkDevice logicalDevice, VkDescriptorSetLayout descriptorSetLayout);

VkInstance NewVkInstance(const char* appName, std::vector<const char*> instanceLayers, std::vector<const char*> instanceExts);

std::vector<VkPhysicalDevice> EnumeratePhysicalDevices(VkInstance vkInstance, uint32_t* gpuCount);

VkDevice NewLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t renderingQueueFamilyIndex, std::vector<const char*> deviceLayers, std::vector<const char*> deviceExts);

VkFormat GetSupportedDepthFormat(VkPhysicalDevice physicalDevice);

VkSemaphore NewSemaphore(VkDevice logicalDevice);

void GetSurfaceColorSpaceAndFormat(VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface,
	VkFormat* surfaceColorFormat,
	VkColorSpaceKHR* surfaceColorSpace);

VkCommandPool NewCommandPool(uint32_t renderingAndPresentingIndex, VkDevice logicalDevice);

VkCommandBuffer NewSetupCommandBuffer(VkDevice logicalDevice, VkCommandPool cmdPool);

void SetImageLayout(VkCommandBuffer cmdBuffer,
	VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout);

std::vector<VkLayerProperties> GetInstalledVkLayers();

std::vector<VkLayerProperties> GetInstalledVkLayers(VkPhysicalDevice physicalDevice);

VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t srcObj,
	size_t location,
	int32_t msgCode,
	const char* layerPrefix,
	const char* msg,
	void* data);

VkCommandBuffer NewCommandBuffer(VkDevice logicalDevice, VkCommandPool cmdPool);

std::vector<VkCommandBuffer> NewCommandBuffer(VkDevice logicalDevice, VkCommandPool cmdPool, uint32_t numBuffers);

void GetMemoryType(VkPhysicalDeviceMemoryProperties memoryProperties, uint32_t typeBits, VkFlags properties, uint32_t* typeIndex);

void setupDepthStencil(VkDevice logicalDevice,
	DepthStencil* depthStencil,
	VkFormat depthFormat,
	uint32_t width,
	uint32_t height,
	VkPhysicalDeviceMemoryProperties memoryProperties,
	VkCommandBuffer setupCmdBuffer);

VkRenderPass NewRenderPass(VkDevice logicalDevice, VkFormat surfaceColorFormat, VkFormat depthFormat);

VkPipelineCache NewPipelineCache(VkDevice logicalDevice);

std::vector<VkFramebuffer> NewFrameBuffer(VkDevice logicalDevice,
	std::vector<SwapChainBuffer> surfaceBuffers,
	VkRenderPass renderPass,
	VkImageView depthStencilView,
	uint32_t numBuffers,
	uint32_t width,
	uint32_t height);

void FlushSetupCommandBuffer(VkDevice logicalDevice, VkCommandPool cmdPool, VkCommandBuffer* setupCmdBuffer, VkQueue queue);

VkBuffer NewBuffer(VkDevice logicalDevice, uint32_t bufferSize, VkBufferUsageFlags usageBits);

StagingBuffer AllocBindDataToGPU(VkDevice logicalDevice, VkPhysicalDeviceMemoryProperties memoryProperties, uint32_t dataSize, void* dataTobind, VkBuffer* buffer, VkDeviceMemory* memory);

VkMemoryAllocateInfo NewMemoryAllocInfo(VkDevice logicalDevice, VkPhysicalDeviceMemoryProperties memoryProperties, VkBuffer buffer, uint32_t desiredAllocSize, VkFlags desiredMemoryProperties);

void DestroyInstance(VkInstance vkInstance, VkDebugReportCallbackEXT debugReport);

void DestroyInstance(VkInstance vkInstance);

VkPipelineShaderStageCreateInfo NewShaderStageInfo(VkDevice logicalDevice, uint32_t* data, uint32_t dataSize, VkShaderStageFlagBits stage);

