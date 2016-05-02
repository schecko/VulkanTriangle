

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#include "util.h"
#include <vector>
#include "win64_vulkantriangle.h"

//function pointers
PFN_vkGetPhysicalDeviceSurfaceSupportKHR GetPhysicalDeviceSurfaceSupportKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR GetPhysicalDeviceSurfaceFormatsKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR GetPhysicalDeviceSurfacePresentModesKHR = nullptr;
PFN_vkCreateSwapchainKHR CreateSwapchainKHR = nullptr;
PFN_vkDestroySwapchainKHR DestroySwapchainKHR = nullptr;
PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR = nullptr;
PFN_vkAcquireNextImageKHR AcquireNextImageKHR = nullptr;
PFN_vkQueuePresentKHR QueuePresentKHR = nullptr;

#define GET_VULKAN_FUNCTION_POINTER_DEV(dev, function)								\
{																					\
	function = (PFN_vk##function) vkGetDeviceProcAddr(dev, "vk"#function);			\
	Assert(function != nullptr, "could not find function "#function);				\
}																					\



VkSurfaceKHR NewSurface(HWND windowHandle, HINSTANCE exeHandle, VkInstance vkInstance)
{
	VkResult error;
	VkSurfaceKHR surface;
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = exeHandle;
	surfaceCreateInfo.hwnd = windowHandle;
	error = vkCreateWin32SurfaceKHR(vkInstance,
		&surfaceCreateInfo,
		nullptr,
		&surface);

	Assert(error, "could not create windows surface");

	//get function pointers after creating instance of vulkan
	GET_VULKAN_FUNCTION_POINTER_INST(vkInstance, GetPhysicalDeviceSurfaceSupportKHR);
	GET_VULKAN_FUNCTION_POINTER_INST(vkInstance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
	GET_VULKAN_FUNCTION_POINTER_INST(vkInstance, GetPhysicalDeviceSurfaceFormatsKHR);
	GET_VULKAN_FUNCTION_POINTER_INST(vkInstance, GetPhysicalDeviceSurfacePresentModesKHR);

	return surface;
}

//from the physicaldevice as a param, returns a grphics queueFamily cabable of a rendering pipeline.
uint32_t FindGraphicsQueueFamilyIndex(VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR surface)
{

	uint32_t queueCount = 0;
	uint32_t graphicsAndPresentingQueueIndex = 0;
	VkBool32 supportsPresent;
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueCount, nullptr);
	Assert(queueCount > 0, "physical device has no available queues");

	std::vector<VkQueueFamilyProperties> queueProperties(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueCount, queueProperties.data());

	for (; graphicsAndPresentingQueueIndex < queueCount; graphicsAndPresentingQueueIndex++)
	{
		GetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, graphicsAndPresentingQueueIndex, surface, &supportsPresent);
		if (queueProperties[graphicsAndPresentingQueueIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT && supportsPresent)
			break;
	}

	Assert(graphicsAndPresentingQueueIndex < queueCount, "this gpu doesn't support graphics rendering.... what a useful gpu");

	return graphicsAndPresentingQueueIndex;
}

void GetSurfaceColorSpaceAndFormat(VkPhysicalDevice physicalDevice,
	SurfaceInfo* surfaceInfo)
{
	uint32_t surfaceFormatCount;
	VkResult error;
	error = GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceInfo->surface, &surfaceFormatCount, nullptr);
	Assert(error, "could not get surface format counts, GetphysicalDeviceSurfaceFormatsKHR is probably null");
	Assert(surfaceFormatCount > 0, "surfaceformatcount is less than 1");

	std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
	error = GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,
		surfaceInfo->surface,
		&surfaceFormatCount,
		surfaceFormats.data());
	Assert(error, "could not get surface format counts, GetphysicalDeviceSurfaceFormatsKHR is probably null");

	if (surfaceFormatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		surfaceInfo->colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	}
	else
	{
		surfaceInfo->colorFormat = surfaceFormats[0].format;
	}
	surfaceInfo->surfaceColorSpace = surfaceFormats[0].colorSpace;
}

static void SetImageLayout(VkCommandBuffer cmdBuffer,
	VkImage image,
	VkImageAspectFlags aspectMask,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout)
{
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	// Undefined layout
	// Only allowed as initial layout!
	// Make sure any writes to the image have been finished
	if (oldImageLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	// Old layout is color attachment
	// Make sure any writes to the color buffer have been finished
	if (oldImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	// Old layout is depth/stencil attachment
	// Make sure any writes to the depth/stencil buffer have been finished
	if (oldImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	// Old layout is transfer source
	// Make sure any reads from the image have been finished
	if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	// Old layout is shader read (sampler, input attachment)
	// Make sure any shader reads from the image have been finished
	if (oldImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	// Target layouts (new)

	// New layout is transfer destination (copy, blit)
	// Make sure any copyies to the image have been finished
	if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	// New layout is transfer source (copy, blit)
	// Make sure any reads from and writes to the image have been finished
	if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = imageMemoryBarrier.srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	// New layout is color attachment
	// Make sure any writes to the color buffer hav been finished
	if (newImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	// New layout is depth attachment
	// Make sure any writes to depth/stencil buffer have been finished
	if (newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	// New layout is shader read (sampler, input attachment)
	// Make sure any writes to the image have been finished
	if (newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	// Put barrier on top
	VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(
		cmdBuffer,
		srcStageFlags,
		destStageFlags,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
}


static VkSurfaceCapabilitiesKHR GetSurfaceCaps(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
	VkSurfaceCapabilitiesKHR surfaceCaps;
	VkResult error;
	error = GetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice,
		surface,
		&surfaceCaps);
	Assert(error, "could not get surface capabilities, the function is probably null?");
	return surfaceCaps;

}

static std::vector<VkPresentModeKHR> GetPresentModes(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{

	uint32_t presentModeCount;
	VkResult error;

	error = GetPhysicalDeviceSurfacePresentModesKHR(physDevice,
		surface,
		&presentModeCount,
		nullptr);
	Assert(error, "could not get surface present modes");
	Assert(presentModeCount > 0, "present mode count is zero!!");

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);


	error = GetPhysicalDeviceSurfacePresentModesKHR(physDevice,
		surface,
		&presentModeCount,
		presentModes.data());
	Assert(error, "could not get the present Modes");
	return presentModes;

}


void InitSwapChain(
	const DeviceInfo* deviceInfo,
	VkPhysicalDevice physDevice,
	SurfaceInfo* surfaceInfo,
	uint32_t* width,
	uint32_t* height)
{
	//TODO parts of this function could be moved to other functions to improve the flow of initialization?
	//specifically move the use of the function extensions into a single function?
	VkResult error;


	//after logical device creation we can retrieve function pointers associated with it
	GET_VULKAN_FUNCTION_POINTER_DEV(deviceInfo->device, CreateSwapchainKHR);
	GET_VULKAN_FUNCTION_POINTER_DEV(deviceInfo->device, DestroySwapchainKHR);
	GET_VULKAN_FUNCTION_POINTER_DEV(deviceInfo->device, GetSwapchainImagesKHR);
	GET_VULKAN_FUNCTION_POINTER_DEV(deviceInfo->device, AcquireNextImageKHR);
	GET_VULKAN_FUNCTION_POINTER_DEV(deviceInfo->device, QueuePresentKHR);

	VkSurfaceCapabilitiesKHR surfaceCaps = GetSurfaceCaps(physDevice, surfaceInfo->surface);
	std::vector<VkPresentModeKHR> presentModes = GetPresentModes(physDevice, surfaceInfo->surface);


	VkExtent2D surfaceExtant = {};
	//width and height are either both -1, or both not -1
	if (surfaceCaps.currentExtent.width == -1)
	{
		surfaceExtant.width = *width;
		surfaceExtant.height = *height;
	}
	else
	{
		surfaceExtant = surfaceCaps.currentExtent;
		*width = surfaceCaps.currentExtent.width;
		*height = surfaceCaps.currentExtent.height;
	}

	VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (size_t i = 0; i < presentModes.size(); i++)
	{
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			swapChainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		if (swapChainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR &&
			presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}

	//determine the number of images
	uint32_t desiredNumberOfSwapChainImages = surfaceCaps.minImageCount + 1;
	if (surfaceCaps.maxImageCount > 0 && desiredNumberOfSwapChainImages > surfaceCaps.maxImageCount)
	{
		desiredNumberOfSwapChainImages = surfaceCaps.maxImageCount;
	}
	VkSurfaceTransformFlagsKHR preTransform;
	if (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = surfaceCaps.currentTransform;
	}

	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.surface = surfaceInfo->surface;
	swapchainCI.minImageCount = desiredNumberOfSwapChainImages;
	swapchainCI.imageFormat = surfaceInfo->colorFormat;
	swapchainCI.imageColorSpace = surfaceInfo->surfaceColorSpace;
	swapchainCI.imageExtent = surfaceExtant;
	swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchainCI.imageArrayLayers = 1;
	swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCI.presentMode = swapChainPresentMode;
	//TODO do I ever have an old swapchain and need to create a new one?
	//swapchainCI.oldSwapchain = oldSwapChain;
	swapchainCI.clipped = VK_TRUE;
	swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	VkSwapchainKHR swapChain;
	error = CreateSwapchainKHR(deviceInfo->device, &swapchainCI, nullptr, &swapChain);
	Assert(error, "could not create a swapchain");

	error = GetSwapchainImagesKHR(deviceInfo->device, swapChain, &surfaceInfo->imageCount, nullptr);
	Assert(error, "could not get surface image count");
	surfaceInfo->images.resize(surfaceInfo->imageCount);
	surfaceInfo->buffers.resize(surfaceInfo->imageCount);
	error = GetSwapchainImagesKHR(deviceInfo->device, swapChain, &surfaceInfo->imageCount, surfaceInfo->images.data());
	Assert(error, "could not fill surface images vector");

	for (uint32_t i = 0; i < surfaceInfo->imageCount; i++)
	{
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.format = surfaceInfo->colorFormat;
		colorAttachmentView.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0;

		surfaceInfo->buffers[i].image = surfaceInfo->images[i];

		SetImageLayout(deviceInfo->setupCmdBuffer,
			surfaceInfo->buffers[i].image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		colorAttachmentView.image = surfaceInfo->buffers[i].image;
		error = vkCreateImageView(deviceInfo->device, &colorAttachmentView, nullptr, &surfaceInfo->buffers[i].view);
		Assert(error, "could not create image view");
	}
}