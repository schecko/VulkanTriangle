#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <windows.h>
#include <vector>
#include <glm/glm.hpp>
#include "commonvulkan.h"
#include "surface.h"

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

	SurfaceInfo surfaceInfo;

	PhysDeviceInfo physDeviceInfo;

	DebugInfo debugInfo;

	DeviceInfo deviceInfo;


	//TODO better name?
	VkSubmitInfo submitInfo;





	DepthStencil depthStencil;



	VkCommandBuffer textureCmdBuffer;
	VertexBuffer vertexBuffer;

	Camera camera;
	PipelineInfo pipelineInfo;


};