#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#include <vulkan/vulkan.h>
#include "commonvulkan.h"

//todo remove cyclic dependance
struct DeviceInfo;

struct SwapChainBuffer
{
	VkImage image;
	VkImageView view;
};

struct SurfaceInfo
{
	VkSurfaceKHR surface;
	VkColorSpaceKHR surfaceColorSpace;
	VkFormat colorFormat;
	uint32_t imageCount;
	std::vector<VkImage> images;
	std::vector<SwapChainBuffer> buffers;
};

VkSurfaceKHR NewSurface(HWND windowHandle, HINSTANCE exeHandle, VkInstance vkInstance);

uint32_t FindGraphicsQueueFamilyIndex(VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR surface);

void GetSurfaceColorSpaceAndFormat(VkPhysicalDevice physicalDevice,
	SurfaceInfo* surfaceInfo);

void InitSwapChain(
	const DeviceInfo* deviceInfo,
	VkPhysicalDevice physDevice,
	SurfaceInfo* surfaceInfo,
	uint32_t* width,
	uint32_t* height);