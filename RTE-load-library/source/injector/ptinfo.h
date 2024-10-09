#pragma once

#ifndef PTINFO
#define PTINFO
#include <windows.h>
#include <TlHelp32.h>
#include <string>

typedef unsigned long ULong;
typedef void* NTPtr;
typedef unsigned char UByte;

uint64_t FindProcessIdFromProcessName(const std::string processName);
uint64_t GetModuleBase(const DWORD dwProcessId, LPCSTR szModuleName);

#endif