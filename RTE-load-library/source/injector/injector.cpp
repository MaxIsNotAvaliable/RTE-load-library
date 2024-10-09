#include "injector.h"
#include "GetProcAddress.h"
#include <filesystem>
#include <files_helper/files.h>

#include <encryption/encstring.h>

#define LOAD_LIBRARY_NAME_W ENCSTR("LoadLibraryW")
#define LOAD_LIBRARY_NAME_A ENCSTR("LoadLibraryA")

#ifdef UNICODE
#define LOAD_LIBRARY_NAME LOAD_LIBRARY_NAME_W
#else
#define LOAD_LIBRARY_NAME LOAD_LIBRARY_NAME_A
#endif

#ifdef _WIN64
using f_Routine = UINT_PTR(__fastcall*)(void* pArg);
#else
using f_Routine = UINT_PTR(__stdcall*)(void* pArg);
#endif

using f_NtCreateThreadEx = NTSTATUS(__stdcall*)(HANDLE* pThreadHandleOut, ACCESS_MASK DesiredAccess, void* pAttr, HANDLE hProc,
	void* pRoutine, void* pArg, ULONG Flags, SIZE_T ZeroBits, SIZE_T StackSize, SIZE_T MaxStackSize, void* pAttrlistOut);


HANDLE GetProcessByName(const TCHAR* szProcName, DWORD dwDesiredAccess = PROCESS_ALL_ACCESS)
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
	{
		return nullptr;
	}

	PROCESSENTRY32 PE32{ 0 };
	PE32.dwSize = sizeof(PE32);

	BOOL bRet = Process32First(hSnap, &PE32);
	while (bRet)
	{
		if (!_tcsicmp(PE32.szExeFile, szProcName))
		{
			break;
		}

		bRet = Process32Next(hSnap, &PE32);
	}

	CloseHandle(hSnap);

	if (!bRet)
		return nullptr;

	return OpenProcess(dwDesiredAccess, FALSE, PE32.th32ProcessID);
}

bool findBytesInProcess(HANDLE hProcess, const BYTE* dllBytes, SIZE_T dllSize) {
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	// Iterate through the memory pages
	MEMORY_BASIC_INFORMATION mbi;
	for (BYTE* address = 0; address < (BYTE*)sysInfo.lpMaximumApplicationAddress; address += mbi.RegionSize) {
		if (VirtualQueryEx(hProcess, address, &mbi, sizeof(mbi))) {
			if (mbi.State == MEM_COMMIT && mbi.Protect != PAGE_NOACCESS) {
				std::vector<BYTE> buffer(mbi.RegionSize);
				SIZE_T bytesRead;

				// Read the memory of the process
				if (ReadProcessMemory(hProcess, address, buffer.data(), mbi.RegionSize, &bytesRead)) {
					// Search for the DLL bytes
					for (SIZE_T i = 0; i <= bytesRead - dllSize; ++i) {
						if (memcmp(buffer.data() + i, dllBytes, dllSize) == 0) {
							std::cout << "Found DLL bytes at address: " << static_cast<void*>(address + i) << std::endl;
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}


std::string CInjector::ReturnCodeToStr(int code)
{
	if (code < 0 || code > 3)
		return EString("Wrong execution code 0x") + EString(code);

	static std::string codes[] = {
		"Success",
		"Already loaded",
		"Dll not found",
		"Process not found",
	};

	return codes[code];
}

bool CInjector::IsDllInjected(DWORD PID, const char* dllPath)
{
	std::string dllName = dllPath;
	if (std::filesystem::path(dllName).is_absolute())
		dllName = std::filesystem::path(dllName).relative_path().string();

	HANDLE h_proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
	if (!h_proc || h_proc == INVALID_HANDLE_VALUE)
		return false;

	HMODULE hDll = GetModuleHandleEx(h_proc, dllName.c_str());

	std::string dllPathStr = dllPath;
	if (std::filesystem::path(dllPathStr).is_relative())
		dllPathStr = manage_files::GetFolder() + "\\" + dllPath;

	if (!manage_files::fileExists(dllPathStr))
	{
		CloseHandle(h_proc);
		return hDll != 0;
	}

	// Читаем файл DLL в байтовый массив
	std::ifstream dllFile(dllPathStr, std::ios::binary);
	std::vector<char> dllBytes((std::istreambuf_iterator<char>(dllFile)), std::istreambuf_iterator<char>());
	dllFile.close();

	//bool found = findBytesInProcess(h_proc, (const BYTE*)dllBytes.data(), MIN<size_t>(1024, dllBytes.size()));

	CloseHandle(h_proc); 
	return hDll != 0;
}

int CInjector::InjectDllByPid(DWORD PID, const char* dllPath)
{
	if (IsDllInjected(PID, dllPath))
		return 1;

	std::string dllPathStr = dllPath;
	if (std::filesystem::path(dllPathStr).is_relative())
		dllPathStr = manage_files::GetFolder() + "\\" + dllPath;

	if (!manage_files::fileExists(dllPathStr))
		return 2;

	const TCHAR* dll_path = dllPathStr.c_str();
	auto len = _tcsclen(dll_path) * sizeof(TCHAR*);
	HANDLE h_proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);

	if (!h_proc || h_proc == INVALID_HANDLE_VALUE)
		return 3;

	const LPVOID nt_open_file = GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), ENCSTR("NtOpenFile"));
	if (nt_open_file)
	{
		char original_bytes[5];
		memcpy(original_bytes, nt_open_file, 5);
		WriteProcessMemory(h_proc, nt_open_file, original_bytes, 5, nullptr);
	}

	auto* loc = VirtualAllocEx(h_proc, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!loc)
	{
		CloseHandle(h_proc);
		return 4;
	}

	if (!WriteProcessMemory(h_proc, loc, dll_path, len, nullptr))
	{
		VirtualFreeEx(h_proc, loc, 0, MEM_RELEASE);
		CloseHandle(h_proc);
		return 5;
	}

	f_Routine* pLoadLibrary = reinterpret_cast<f_Routine*>(GetProcAddressEx(h_proc, TEXT("kernel32.dll"), LOAD_LIBRARY_NAME));
	f_NtCreateThreadEx p_NtCreateThreadEx = (f_NtCreateThreadEx)(GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), ENCSTR("NtCreateThreadEx")));

	if (!pLoadLibrary || !p_NtCreateThreadEx)
	{
		VirtualFreeEx(h_proc, loc, 0, MEM_RELEASE);
		CloseHandle(h_proc);
		return 6;
	}

	HANDLE h_thread = nullptr;
	NTSTATUS ntRet = p_NtCreateThreadEx(&h_thread, THREAD_ALL_ACCESS, nullptr, h_proc, pLoadLibrary, loc, 0, 0, 0, 0, nullptr);
	if (ntRet < 0 || !h_thread)
	{
		VirtualFreeEx(h_proc, loc, 0, MEM_RELEASE);
		CloseHandle(h_proc);
		return 7;
	}

	CloseHandle(h_thread);
	CloseHandle(h_proc);
	return 0;
}

//int CInjector::InjectDllByProcName(const char* processName, const char* dllPath)
//{
//	std::string dllPathStr = dllPath;
//	if (std::filesystem::path(dllPathStr).is_relative())
//		dllPathStr = manage_files::GetFolder() + "\\" + dllPath;
//
//	if (!manage_files::fileExists(dllPathStr))
//		return 1;
//
//	const TCHAR* dll_path = dllPath;
//	auto len = _tcsclen(dll_path) * sizeof(TCHAR*);
//	HANDLE h_proc = GetProcessByName(processName);
//
//	if (!h_proc || h_proc == INVALID_HANDLE_VALUE)
//		return 1;
//
//	const LPVOID nt_open_file = GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "NtOpenFile");
//	if (nt_open_file)
//	{
//		char original_bytes[5];
//		memcpy(original_bytes, nt_open_file, 5);
//		WriteProcessMemory(h_proc, nt_open_file, original_bytes, 5, nullptr);
//	}
//
//	auto* loc = VirtualAllocEx(h_proc, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//	if (!loc)
//	{
//		CloseHandle(h_proc);
//		return 2;
//	}
//
//	if (!WriteProcessMemory(h_proc, loc, dll_path, len, nullptr))
//	{
//		VirtualFreeEx(h_proc, loc, 0, MEM_RELEASE);
//		CloseHandle(h_proc);
//		return 3;
//	}
//
//	f_Routine* pLoadLibrary = reinterpret_cast<f_Routine*>(GetProcAddressEx(h_proc, TEXT("kernel32.dll"), LOAD_LIBRARY_NAME));
//	f_NtCreateThreadEx p_NtCreateThreadEx = (f_NtCreateThreadEx)(GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "NtCreateThreadEx"));
//
//	if (!pLoadLibrary || !p_NtCreateThreadEx)
//	{
//		VirtualFreeEx(h_proc, loc, 0, MEM_RELEASE);
//		CloseHandle(h_proc);
//		return 5;
//	}
//
//	HANDLE h_thread = nullptr;
//	NTSTATUS ntRet = p_NtCreateThreadEx(&h_thread, THREAD_ALL_ACCESS, nullptr, h_proc, pLoadLibrary, loc, 0, 0, 0, 0, nullptr);
//	if (ntRet < 0 || !h_thread)
//	{
//		VirtualFreeEx(h_proc, loc, 0, MEM_RELEASE);
//		CloseHandle(h_proc);
//		return 5;
//	}
//
//	CloseHandle(h_thread);
//	CloseHandle(h_proc);
//	return 0;
//}
