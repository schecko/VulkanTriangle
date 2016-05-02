

#include <windows.h>
#include <string>
#include "commonwindows.h"
#include "util.h"
#include "win64_vulkantriangle.h"

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

//create a windows window, pass a pointer to a struct for input events
//returns a handle to the created window.
HWND NewWindow(const char* appName, void* pointer, uint32_t clientWidth, uint32_t clientHeight)
{


	WNDCLASS wc = {};

	wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MessageHandler;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.lpszClassName = appName;
	wc.lpszMenuName = appName;
	wc.cbWndExtra = sizeof(void*);


	ATOM result = 0;
	result = RegisterClass(&wc);

	Assert(result != 0, "could not register windowclass");


	//TODO calculate windowrect from clientrect
	uint32_t windowWidth = clientWidth;
	uint32_t windowHeight = clientHeight;

	HWND windowHandle = CreateWindow(appName,
		appName,
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




File OpenFile(std::string fileName)
{
	File file = {};

	HANDLE fileHandle = CreateFileA(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(fileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(fileHandle, &fileSize))
		{
			Assert(fileSize.QuadPart < (int64_t)pow(2, 32), "file too big");
			uint32_t fileSize32 = fileSize.QuadPart;
			file.data = VirtualAlloc(0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if(file.data)
			{
				DWORD bytesRead;
				if(ReadFile(fileHandle, file.data, fileSize32, &bytesRead, 0) && fileSize32 == bytesRead)
				{
					file.size = fileSize32;
				}
				else
				{
					VirtualFree(file.data, 0, MEM_RELEASE);
					file.data = nullptr;
				}
			}

		}
	}
	Assert(file.data != nullptr, "could not read file");
	return file;
	
}