#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "win64_vulkantriangle.h" //TODO remove

VkSurfaceKHR NewSurface(HWND windowHandle, HINSTANCE exeHandle, VkInstance vkInstance);

uint32_t FindGraphicsQueueFamilyIndex(VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR surface);

void GetSurfaceColorSpaceAndFormat(VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface,
	VkFormat* surfaceColorFormat,
	VkColorSpaceKHR* surfaceColorSpace);

void InitSwapChain(
	/*VkDevice logicalDevice,
	VkPhysicalDevice physicalDevice,
	VkSurfaceKHR surface,
	VkFormat imageFormat,
	VkColorSpaceKHR colorSpace,
	*/
	MainMemory* mainMemory,
	uint32_t* width,
	uint32_t* height);