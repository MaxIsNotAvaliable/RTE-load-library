#pragma once
#include "base.h"

class ResourceManager
{
public:
	// Returns void*, size_t of resource
	static void* GetResourceBrute(unsigned int IDR, uintptr_t& sizeOut);
	static void* GetResourceBruteEx(const char* moduleName, unsigned int IDR, uintptr_t& sizeOut);
#ifdef UNICODE
	static void* GetResource(unsigned int IDR, LPCWSTR RT_TYPE, uintptr_t& sizeOut);
	static void* GetResourceEx(const char* moduleName, unsigned int IDR, LPCWSTR RT_TYPE, uintptr_t& sizeOut);
#else
	static void* GetResource(unsigned int IDR, const char* RT_TYPE, uintptr_t& sizeOut);
	static void* GetResourceEx(const char* moduleName, unsigned int IDR, const char* RT_TYPE, uintptr_t& sizeOut);
#endif


	static void* GetFont(unsigned int IDR, uintptr_t& sizeOut);
	static void* GetImagePNG(unsigned int IDR, uintptr_t& sizeOut);
};

