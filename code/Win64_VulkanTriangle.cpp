

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

#define VALIDATION_LAYERS false

#if VALIDATION_LAYERS
#include <vulkan/vk_layer.h>
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
    VkPhysicalDevice vkPhysicalDevice;


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

//bare bones wndproc for now
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
    wc.cbClsExtra = sizeof(void*);


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
                                     &pointer);

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
    int validationLayerCount = 9;
    instanceCreateInfo.enabledLayerCount = validationLayerCount;
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

//windows entrypoint, replacing with winmain seems redundant to me (plus I dont remember its function signature)
int main(int argv, char** argc)
{
    MainMemory* mainMemory = new MainMemory();
    mainMemory->running = true;
    mainMemory->consoleHandle = GetConsoleWindow();
    ShowWindow(mainMemory->consoleHandle, SW_HIDE);

    mainMemory->windowHandle = NewWindow(mainMemory);
    mainMemory->vkInstance = NewVkInstance();

    uint32_t gpuCount;
    std::vector<VkPhysicalDevice> physicalDevices = EnumeratePhysicalDevices(mainMemory->vkInstance, &gpuCount);
    mainMemory->vkPhysicalDevice = physicalDevices[0];



    ///
    //physical device
    ///

    ///
    // end device
    ///

    //get function pointers after creating instance of vulkan
    GET_VULKAN_FUNCTION_POINTER_INST(mainMemory->vkInstance, GetPhysicalDeviceSurfaceSupportKHR);
    GET_VULKAN_FUNCTION_POINTER_INST(mainMemory->vkInstance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_VULKAN_FUNCTION_POINTER_INST(mainMemory->vkInstance, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_VULKAN_FUNCTION_POINTER_INST(mainMemory->vkInstance, GetPhysicalDeviceSurfacePresentModesKHR);


    //after logical device creation we can retrieve function pointers associated with it
    //GET_VULKAN_FUNCTION_POINTER_DEV(mainMemory->logicalDevice, CreateSwapchainKHR);
    //GET_VULKAN_FUNCTION_POINTER_DEV(mainMemory->logicalDevice, DestroySwapchainKHR);
    //GET_VULKAN_FUNCTION_POINTER_DEV(mainMemory->logicalDevice, GetSwapchainImagesKHR);
    //GET_VULKAN_FUNCTION_POINTER_DEV(mainMemory->logicalDevice, AcquireNextImageKHR);
    //GET_VULKAN_FUNCTION_POINTER_DEV(mainMemory->logicalDevice, QueuePresentKHR);





    ///
    //MAIN LOOP
    ///
    while (mainMemory->running)
    {
        MSG msg;
        while (PeekMessage(&msg, mainMemory->windowHandle, 0, 0, PM_REMOVE))
        {

            TranslateMessage(&msg);
            DispatchMessage(&msg);

        } //message processing










    }


    return 0;
}

//VkSurfaceKHR NewVkSurface(HWND windowHandle, VkInstance vkInstance)
//{
//	VkResult error;
//	VkSurfaceKHR surface;
//	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
//	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
//	surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
//	surfaceCreateInfo.hwnd = windowHandle;
//	error = vkCreateWin32SurfaceKHR(vkInstance,
//		&surfaceCreateInfo,
//		nullptr,
//		&surface);
//
//	Assert(error == VK_SUCCESS, "could not create windows surface");
//
//	return surface;
//}