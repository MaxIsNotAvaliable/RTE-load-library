#pragma once
#include <base.h>

enum ARCH : int
{
	ARCH_UNKNOWN	= (1 << 0),
	ARCH_x86		= (1 << 1),
	ARCH_x64		= (1 << 2),
};

struct ProcessInfo_t
{
	DWORD pid;
	EString processName;
	ARCH arch;
};

class CProcessManager
{
private:
	static inline std::vector<ProcessInfo_t> processList;

public:
	static void UpdateProcessList();
	static std::vector<ProcessInfo_t> GetProcessList(const std::string& filterByName = "\0");
	static ProcessInfo_t GetProcessByPid(DWORD pid);
};

