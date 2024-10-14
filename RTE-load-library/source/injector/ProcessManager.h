#pragma once
#include <base.h>

struct ProcessInfo_t
{
	DWORD pid;
	EString processName;
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

