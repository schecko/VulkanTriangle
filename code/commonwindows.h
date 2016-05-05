#pragma once


#include <windows.h>


struct File
{
	void* data;
	uint32_t size;
};

enum InputKeys
{
	up,
	left,
	down,
	right,

	//KEEP THIS AT THE BOTTOM ALWAYS.
	size
};

struct Input
{
	bool running;
	bool keys[InputKeys::size];

};

HWND NewWindow(const char* appName, void* pointer, uint32_t clientWidth, uint32_t clientHeight);

File OpenFile(std::string fileName);