#pragma once

#include <string>
#include <vulkan/vulkan.h>
#include <iostream>

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
}																					\

//assert the test is true, if test is false the console is moved to the focus,
//the message is printed out and execution halts using std::cin rather than abort() or nullptr errors
inline void Assert(bool test, std::string message)
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
		//ShowWindow(consoleHandle, SW_HIDE);
	}
#endif
}

inline void Assert(VkResult test, std::string message)
{
	Assert(test == VK_SUCCESS, message);
}

inline void Message(std::string message)
{
	std::cout << message.c_str() << std::endl;
}

inline void Message(uint32_t message)
{
	std::cout << std::to_string(message).c_str() << std::endl;
}