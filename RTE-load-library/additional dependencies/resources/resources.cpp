#include "resources.h"

#define PROJECT_NAME "RTE-load-library.exe"

void* ResourceManager::GetResourceBrute(unsigned int IDR, uintptr_t& sizeOut)
{
	return GetResourceBruteEx(PROJECT_NAME, IDR, sizeOut);
}

void* ResourceManager::GetResourceBruteEx(const char* moduleName, unsigned int IDR, uintptr_t& sizeOut)
{
	HMODULE hModule = GetModuleHandleA(moduleName);
	if (hModule == NULL) return nullptr;

	HRSRC hResource = nullptr;
	for (uintptr_t i = 1; i < 25 && !hResource; i++)
	{
		hResource = FindResource(hModule, MAKEINTRESOURCE(IDR), MAKEINTRESOURCE(i));
		if (hResource) break;
	}
	if (hResource == NULL) return nullptr;

	HGLOBAL resFile = LoadResource(hModule, hResource);
	if (resFile == NULL) return nullptr;

	uintptr_t resSize = SizeofResource(hModule, hResource);
	if (resSize == NULL) return nullptr;

	void* resData = LockResource(resFile);
	if (resData == NULL) return nullptr;

	sizeOut = resSize;
	return resData;
}

#ifdef UNICODE
void* ResourceManager::GetResource(unsigned int IDR, LPCWSTR RT_TYPE, uintptr_t& sizeOut)
#else
void* ResourceManager::GetResource(unsigned int IDR, const char* RT_TYPE, uintptr_t& sizeOut)
#endif
{
	return GetResourceEx(PROJECT_NAME, IDR, RT_TYPE, sizeOut);
}

#ifdef UNICODE
void* ResourceManager::GetResourceEx(const char* moduleName, unsigned int IDR, LPCWSTR RT_TYPE, uintptr_t& sizeOut)
#else
void* ResourceManager::GetResourceEx(const char* moduleName, unsigned int IDR, const char* RT_TYPE, uintptr_t& sizeOut)
#endif
{
	HMODULE hModule = GetModuleHandleA(moduleName);
	if (hModule == NULL) return nullptr;

	HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDR), RT_TYPE);
	if (hResource == NULL) return nullptr;

	HGLOBAL resFile = LoadResource(hModule, hResource);
	if (resFile == NULL) return nullptr;

	uintptr_t resSize = SizeofResource(hModule, hResource);
	if (resSize == NULL) return nullptr;

	void* resData = LockResource(resFile);
	if (resData == NULL) return nullptr;

	sizeOut = resSize;
	return resData;
}

void* ResourceManager::GetFont(unsigned int IDR, uintptr_t& sizeOut)
{
	return GetResource(IDR, RT_FONT, sizeOut);
}

void* ResourceManager::GetImagePNG(unsigned int IDR, uintptr_t& sizeOut)
{
	uintptr_t dwResourceSize = 0;
	void* pResourceData = GetResource(IDR, TEXT("PNG"), dwResourceSize);
	sizeOut = dwResourceSize;
	return pResourceData;
}
