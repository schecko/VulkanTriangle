

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

#define VALIDATION_LAYERS false

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

//main struct, pretty much holds everything due to the simple nature of this program
struct MainMemory
{
    HWND consoleHandle;
    HWND windowHandle;
    HINSTANCE exeHandle;

    bool running;

    VkInstance vkInstance;

    VkSurfaceKHR surface;
    VkColorSpaceKHR surfaceColorSpace;
    VkFormat surfaceColorFormat;

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

//assert the test is true, if test is false the console is moved to the front, the message is printed out and execution halts using std::cin
void Assert(bool test, std::string message)
{
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
}


//create a windows window, pass a pointer to a struct required in the wndproc function.
//returns a handle to the created window.
HWND NewWindow(void* pointer)
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


    HWND windowHandle = CreateWindow(EXE_NAME,
                                     EXE_NAME,
                                     WS_OVERLAPPEDWINDOW,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     nullptr,
                                     nullptr,
                                     wc.hInstance,
                                     pointer);

    Assert(windowHandle != nullptr, "could not create a windows window");

    ShowWindow(windowHandle, SW_SHOW);

    return windowHandle;
}

//create the vulkan instance. includes some debug code but its fairly broken....
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

VkCommandBuffer NewSetupBuffer(VkDevice logicalDevice, VkCommandPool cmdPool)
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
    //error = vkBeginCommandBuffer(setup);

}

void Init(MainMemory* mainMemory)
{

    mainMemory->running = true;
    mainMemory->consoleHandle = GetConsoleWindow();
    ShowWindow(mainMemory->consoleHandle, SW_HIDE);

    mainMemory->windowHandle = NewWindow(mainMemory);
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

