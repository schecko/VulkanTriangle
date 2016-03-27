

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

#define VALIDATION_LAYERS false
#define DEBUGGING true

#if VALIDATION_LAYERS
#include <vulkan/vk_layer.h>
int validationLayerCount = 9;
const char *validationLayerNames[] =
{
    "VK_LAYER_GOOGLE_threading",
    "VK_LAYER_LUNARG_mem_tracker",
    "VK_LAYER_LUNARG_object_tracker",
    "VK_LAYER_LUNARG_draw_state",
    "VK_LAYER_LUNARG_param_checker",
    "VK_LAYER_LUNARG_swapchain",
    "VK_LAYER_LUNARG_device_limits",
    "VK_LAYER_LUNARG_image",
    "VK_LAYER_GOOGLE_unique_objects",
};
#endif

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

const char* EXE_NAME = "VulkanTriangle";
struct SwapChainBuffer
{
    VkImage image;
    VkImageView view;
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

    VkDevice logicalDevice;
    VkQueue queue;
    VkCommandPool cmdPool;

    VkSemaphore presentComplete;
    VkSemaphore renderComplete;

    //TODO better name?
    VkSubmitInfo submitInfo;

    VkCommandBuffer setupCommandBuffer;


};

//helper macros for annoying function pointers
#define GET_VULKAN_FUNCTION_POINTER_INST(inst, function)							\
{																					\
	function = (PFN_vk##function) vkGetInstanceProcAddr(inst, "vk"#function);		\
	Assert(function != nullptr, "could not find function "#function);				\
}																					\

#define GET_VULKAN_FUNCTION_POINTER_DEV(dev, function)								\
{																					\
	function = (PFN_vk##function) vkGetDeviceProcAddr(dev, "vk"#function);			\
	Assert(function != nullptr, "could not find function "#function);				\
}

//handle the windows messages
LRESULT CALLBACK MessageHandler(HWND hwnd, UINT msg, WPARAM wP, LPARAM lP)
{

    MainMemory* mainMemory = (MainMemory*)GetWindowLongPtr(hwnd, 0);
    switch (msg)
    {
        case WM_CREATE:
        {
            CREATESTRUCTW* cs = (CREATESTRUCTW*)lP;
            SetWindowLongPtrW(hwnd, 0, (LONG_PTR)cs->lpCreateParams);
        }
        break;
        case WM_DESTROY:
        case WM_CLOSE:
        {
            mainMemory->running = false;
        }
        break;
        default:
        {
            return DefWindowProc(hwnd, msg, wP, lP);
        }
    }
    return 0;
}

//assert the test is true, if test is false the console is moved to the focus,
//the message is printed out and execution halts using std::cin rather than abort() or nullptr errors
void Assert(bool test, std::string message)
{
#if DEBUGGING == true
    char x;
    if (test == false)
    {
        HWND consoleHandle = GetConsoleWindow();
        ShowWindow(consoleHandle, SW_SHOW);
        SetFocus(consoleHandle);
        std::cout << message.c_str() << std::endl;
        std::cout << "Type anything and press enter if you wish to continue anyway; best to exit though." << std::endl;
        std::cin >> x;
        ShowWindow(consoleHandle, SW_HIDE);
    }
#endif
}


//create a windows window, pass a pointer to a struct for input events
//returns a handle to the created window.
HWND NewWindow(void* pointer, uint32_t clientWidth, uint32_t clientHeight)
{


    WNDCLASS wc = {};

    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MessageHandler;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = EXE_NAME;
    wc.lpszMenuName = EXE_NAME;
    wc.cbWndExtra = sizeof(void*);


    ATOM result = 0;
    result = RegisterClass(&wc);

    Assert(result != 0, "could not register windowclass");


    //TODO calculate windowrect from clientrect
    uint32_t windowWidth = clientWidth;
    uint32_t windowHeight = clientHeight;

    HWND windowHandle = CreateWindow(EXE_NAME,
                                     EXE_NAME,
                                     WS_OVERLAPPEDWINDOW,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     windowWidth,
                                     windowHeight,
                                     nullptr,
                                     nullptr,
                                     wc.hInstance,
                                     pointer);

    Assert(windowHandle != nullptr, "could not create a windows window");

    ShowWindow(windowHandle, SW_SHOW);

    return windowHandle;
}

//create the vulkan instance.
//TODO learn the validation layers
//returns the created instance of vulkan
VkInstance NewVkInstance()
{
    VkResult error;
    VkInstance vkInstance;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = EXE_NAME;
    appInfo.pEngineName = EXE_NAME;
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 2);

    std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME };
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);


    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    if (extensions.size() > 0)
    {

#if VALIDATION_LAYERS
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif
        instanceCreateInfo.enabledExtensionCount = (uint32_t)extensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
    }
#if(VALIDATION_LAYERS)
    //TODO figure out how to use them
    instanceCreateInfo.enabledLayerCount = validationLayerCount;
    instanceCreateInfo.ppEnabledLayerNames = validationLayerNames;
#endif

    error = vkCreateInstance(&instanceCreateInfo, nullptr, &vkInstance);

    Assert(error == VK_SUCCESS, "could not create instance of vulkan");

    return vkInstance;
}


//fill the gpuCount param with the number of physical devices available to the program, and return a pointer to a vector containing the physical devices
//returns a vector of the physical devices handles.
std::vector<VkPhysicalDevice> EnumeratePhysicalDevices(VkInstance vkInstance, uint32_t* gpuCount)
{

    VkResult error;
    error = vkEnumeratePhysicalDevices(vkInstance, gpuCount, nullptr);
    Assert(error == VK_SUCCESS, "could not enumerate gpus");
    Assert(gpuCount > 0, "no compatible vulkan gpus available");

    std::vector<VkPhysicalDevice> physicalDevices(*gpuCount);
    error = vkEnumeratePhysicalDevices(vkInstance, gpuCount, physicalDevices.data());
    Assert(error == VK_SUCCESS, "could not enumerate physical devices (2nd usage of 2)");

    return physicalDevices;
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

VkDevice NewLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t renderingQueueFamilyIndex)
{
    float queuePriorities[1] = { 0.0f };
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = renderingQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = queuePriorities;

    std::vector<const char*> enabledExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
#if VALIDATION_LAYERS
    deviceCreateInfo.enabledLayerCount = validationLayerCount;
    deviceCreateInfo.ppEnabledLayerNames = validationLayerNames;
#endif

    VkResult error;
    VkDevice logicalDevice;
    error = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);

    Assert(error == VK_SUCCESS, "could not create logical device");
    return logicalDevice;
}

VkFormat GetSupportedDepthFormat(VkPhysicalDevice physicalDevice)
{
    VkFormat depthFormats[] =
    {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };
    VkFormat depthFormat;
    bool found = false;
    for (uint32_t i = 0; i < sizeof(depthFormats) / sizeof(VkFormat); i++)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, depthFormats[i], &formatProps);
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            depthFormat = depthFormats[i];
            found = true;
        }

    }
    Assert(found == true, "could not find a supported depth format");
    return depthFormat;
}

VkSemaphore NewSemaphore(VkDevice logicalDevice)
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkSemaphore semaphore;
    VkResult error = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &semaphore);
    Assert(error == VK_SUCCESS, "could not create semaphore");
    return semaphore;
}

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

    Assert(error == VK_SUCCESS, "could not create windows surface");

    return surface;
}

void GetSurfaceColorSpaceAndFormat(VkPhysicalDevice physicalDevice,
                                   VkSurfaceKHR surface,
                                   VkFormat* surfaceColorFormat,
                                   VkColorSpaceKHR* surfaceColorSpace)
{
    uint32_t surfaceFormatCount;
    VkResult error;
    error = GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);
    Assert(error == VK_SUCCESS, "could not get surface format counts, GetphysicalDeviceSurfaceFormatsKHR is probably null");
    Assert(surfaceFormatCount > 0, "surfaceformatcount is less than 1");

    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    error = GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,
            surface,
            &surfaceFormatCount,
            surfaceFormats.data());
    Assert(error == VK_SUCCESS, "could not get surface format counts, GetphysicalDeviceSurfaceFormatsKHR is probably null");

    if (surfaceFormatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        *surfaceColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        *surfaceColorFormat = surfaceFormats[0].format;
    }
    *surfaceColorSpace = surfaceFormats[0].colorSpace;
}

VkCommandPool NewCommandPool(uint32_t renderingAndPresentingIndex, VkDevice logicalDevice)
{
    VkResult error;
    VkCommandPool pool;
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = renderingAndPresentingIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    error = vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &pool);
    Assert(error == VK_SUCCESS, "could not create command pool.");
    return pool;
}

VkCommandBuffer NewSetupCommandBuffer(VkDevice logicalDevice, VkCommandPool cmdPool)
{
    VkCommandBuffer setupBuffer;
    VkResult error;
    VkCommandBufferAllocateInfo bufferAllocInfo = {};
    bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocInfo.commandPool = cmdPool;
    bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocInfo.commandBufferCount = 1;


    error = vkAllocateCommandBuffers(logicalDevice, &bufferAllocInfo, &setupBuffer);
    Assert(error == VK_SUCCESS, "could not create setup command buffer");
    VkCommandBufferBeginInfo setupBeginInfo = {};
    setupBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    error = vkBeginCommandBuffer(setupBuffer, &setupBeginInfo);
    Assert(error == VK_SUCCESS, "could not begin setup command buffer.");
    return setupBuffer;

}

//TODO this function is a little rediculous, simplify into smaller functions or something?
void InitSwapChain(
    /*VkDevice logicalDevice,
      VkPhysicalDevice physicalDevice,
      VkSurfaceKHR surface,
      VkFormat imageFormat,
      VkColorSpaceKHR colorSpace,
      */
    MainMemory* mainMemory,
    uint32_t* width,
    uint32_t* height)
{
    //TODO parts of this function could be moved to other functions to improve the flow of initialization?
    //specifically move the use of the function extensions into a single function?
    VkResult error;
    VkSwapchainKHR swapChain;


    VkSurfaceCapabilitiesKHR surfaceCaps;
    error = GetPhysicalDeviceSurfaceCapabilitiesKHR(mainMemory->physicalDevice,
            mainMemory->surface,
            &surfaceCaps);
    Assert(error == VK_SUCCESS, "could not get surface capabilities, the function is probably null?");

    uint32_t presentModeCount;
    error = GetPhysicalDeviceSurfacePresentModesKHR(mainMemory->physicalDevice,
            mainMemory->surface,
            &presentModeCount,
            nullptr);
    Assert(error == VK_SUCCESS, "could not get surface present modes");
    Assert(presentModeCount > 0, "present mode count is zero!!");

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);


    error = GetPhysicalDeviceSurfacePresentModesKHR(mainMemory->physicalDevice,
            mainMemory->surface,
            &presentModeCount,
            presentModes.data());
    Assert(error == VK_SUCCESS, "could not get the present Modes");

    VkExtent2D swapChainExtent = {};
    //width and height are either both -1, or both not -1
    if (surfaceCaps.currentExtent.width == -1)
    {
        swapChainExtent.width = *width;
        swapChainExtent.height = *height;
    }
    else
    {
        swapChainExtent = surfaceCaps.currentExtent;
        *width = surfaceCaps.currentExtent.width;
        *height = surfaceCaps.currentExtent.height;
    }

    VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (size_t i = 0; i < presentModeCount; i++)
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
    //TODO what is this???
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
    swapchainCI.surface = mainMemory->surface;
    swapchainCI.minImageCount = desiredNumberOfSwapChainImages;
    swapchainCI.imageFormat = mainMemory->surfaceColorFormat;
    swapchainCI.imageColorSpace = mainMemory->surfaceColorSpace;
    swapchainCI.imageExtent = {swapChainExtent.width, swapChainExtent.height};
    swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCI.presentMode = swapChainPresentMode;
    //TODO do I ever have an old swapchain and need to create a new one?
    //swapchainCI.oldSwapchain = oldSwapChain;
    swapchainCI.clipped = VK_TRUE;
    swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    error = CreateSwapchainKHR(mainMemory->logicalDevice, &swapchainCI, nullptr, &swapChain);
    Assert(error == VK_SUCCESS, "could not create a swapchain");

    error = GetSwapchainImagesKHR(mainMemory->logicalDevice, swapChain, &mainMemory->surfaceImageCount, nullptr);
    Assert(error == VK_SUCCESS, "could not get surface image count");
    mainMemory->surfaceImages.resize(mainMemory->surfaceImageCount);
    mainMemory->surfaceBuffers.resize(mainMemory->surfaceImageCount);
    error = GetSwapchainImagesKHR(mainMemory->logicalDevice, swapChain, &mainMemory->surfaceImageCount, mainMemory->surfaceImages.data());
    Assert(error == VK_SUCCESS, "could not fill surface images vector");




}

void Init(MainMemory* mainMemory)
{

    mainMemory->running = true;
    mainMemory->consoleHandle = GetConsoleWindow();
    ShowWindow(mainMemory->consoleHandle, SW_HIDE);
    mainMemory->clientWidth = 1200;
    mainMemory->clientHeight = 800;

    mainMemory->windowHandle = NewWindow(mainMemory, mainMemory->clientWidth, mainMemory->clientHeight);
    mainMemory->vkInstance = NewVkInstance();
    mainMemory->surface = NewSurface(mainMemory->windowHandle, mainMemory->exeHandle, mainMemory->vkInstance);

    //get function pointers after creating instance of vulkan
    GET_VULKAN_FUNCTION_POINTER_INST(mainMemory->vkInstance, GetPhysicalDeviceSurfaceSupportKHR);
    GET_VULKAN_FUNCTION_POINTER_INST(mainMemory->vkInstance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_VULKAN_FUNCTION_POINTER_INST(mainMemory->vkInstance, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_VULKAN_FUNCTION_POINTER_INST(mainMemory->vkInstance, GetPhysicalDeviceSurfacePresentModesKHR);

    uint32_t gpuCount;
    std::vector<VkPhysicalDevice> physicalDevices = EnumeratePhysicalDevices(mainMemory->vkInstance, &gpuCount);
    mainMemory->physicalDevice = physicalDevices[0];

    //see what the gpu is capable of
    //NOTE not actually used in other code
    vkGetPhysicalDeviceFeatures(mainMemory->physicalDevice, &mainMemory->deviceFeatures);

    vkGetPhysicalDeviceMemoryProperties(mainMemory->physicalDevice, &mainMemory->memoryProperties);

    mainMemory->renderingQueueFamilyIndex = FindGraphicsQueueFamilyIndex(mainMemory->physicalDevice, mainMemory->surface);
    mainMemory->logicalDevice = NewLogicalDevice(mainMemory->physicalDevice, mainMemory->renderingQueueFamilyIndex);
    vkGetDeviceQueue(mainMemory->logicalDevice, mainMemory->renderingQueueFamilyIndex, 0, &mainMemory->queue);
    mainMemory->supportedDepthFormat = GetSupportedDepthFormat(mainMemory->physicalDevice);


    //after logical device creation we can retrieve function pointers associated with it
    GET_VULKAN_FUNCTION_POINTER_DEV(mainMemory->logicalDevice, CreateSwapchainKHR);
    GET_VULKAN_FUNCTION_POINTER_DEV(mainMemory->logicalDevice, DestroySwapchainKHR);
    GET_VULKAN_FUNCTION_POINTER_DEV(mainMemory->logicalDevice, GetSwapchainImagesKHR);
    GET_VULKAN_FUNCTION_POINTER_DEV(mainMemory->logicalDevice, AcquireNextImageKHR);
    GET_VULKAN_FUNCTION_POINTER_DEV(mainMemory->logicalDevice, QueuePresentKHR);


    mainMemory->presentComplete = NewSemaphore(mainMemory->logicalDevice);
    mainMemory->renderComplete = NewSemaphore(mainMemory->logicalDevice);

    mainMemory->submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    mainMemory->submitInfo.pWaitDstStageMask = (VkPipelineStageFlags*)VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    mainMemory->submitInfo.waitSemaphoreCount = 1;
    mainMemory->submitInfo.pWaitSemaphores = &mainMemory->presentComplete;
    mainMemory->submitInfo.signalSemaphoreCount = 1;
    mainMemory->submitInfo.pSignalSemaphores = &mainMemory->renderComplete;


    GetSurfaceColorSpaceAndFormat(mainMemory->physicalDevice,
                                  mainMemory->surface,
                                  &mainMemory->surfaceColorFormat,
                                  &mainMemory->surfaceColorSpace);

    mainMemory->cmdPool = NewCommandPool(mainMemory->renderingQueueFamilyIndex, mainMemory->logicalDevice);
    mainMemory->setupCommandBuffer = NewSetupCommandBuffer(mainMemory->logicalDevice,
                                     mainMemory->cmdPool);

    InitSwapChain(mainMemory, &mainMemory->clientWidth, &mainMemory->clientHeight);
}

void UpdateAndRender(MainMemory* mainMemory)
{

}

void PollEvents(HWND windowHandle)
{
    MSG msg;
    while (PeekMessage(&msg, windowHandle, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

    }
}

void Quit(MainMemory* mainMemory)
{
    DestroyWindow(mainMemory->windowHandle);
}
//windows entrypoint, replacing with winmain seems redundant to me (plus its function signature is annoying to remember)
int main(int argv, char** argc)
{

    MainMemory* mainMemory = new MainMemory();
    Init(mainMemory);
    while (mainMemory->running)
    {
        PollEvents(mainMemory->windowHandle);
        UpdateAndRender(mainMemory);
    }
    Quit(mainMemory);
    delete mainMemory;
    return 0;
}

