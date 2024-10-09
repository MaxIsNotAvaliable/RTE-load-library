#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <vector>
#include <string>

HINSTANCE GetModuleHandleEx(HANDLE hTargetProc, const TCHAR* lpModuleName);
void* GetProcAddressEx(HANDLE hTargetProc, const TCHAR* lpModuleName, const char* lpProcName);
std::vector<std::string> GetAllFunctions(HANDLE hTargetProc, const TCHAR* lpModuleName);