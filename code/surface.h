#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "commonvulkan.h"

//todo remove cyclic dependance?
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
	VkFormat surfaceColorFormat;
	uint32_t surfaceImageCount;
	std::vector<VkImage> surfaceImages;
	std::vector<SwapChainBuffer> surfaceBuffers;
};

VkSurfaceKHR NewSurface(HWND windowHandle, HINSTANCE exeHandle, VkInstance vkInstance);

uint32_t FindGraphicsQueueFamilyIndex(VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR surface);

void GetSurfaceColorSpaceAndFormat(VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface,
	VkFormat* surfaceColorFormat,
	VkColorSpaceKHR* surfaceColorSpace);

void InitSwapChain(
	DeviceInfo* deviceInfo,
	VkPhysicalDevice physDevice,
	SurfaceInfo* surfaceInfo,
	uint32_t* width,
	uint32_t* height);