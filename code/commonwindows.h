#pragma once


#include <windows.h>


struct File
{
	void* data;
	uint32_t size;
};

HWND NewWindow(const char* appName, void* pointer, uint32_t clientWidth, uint32_t clientHeight);

File OpenFile(std::string fileName);