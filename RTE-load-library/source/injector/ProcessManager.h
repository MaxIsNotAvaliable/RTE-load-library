#pragma once
#include <base.h>

enum ARCH : int
{
	ARCH_UNKNOWN = (1 << 0),
	ARCH_x86 = (1 << 1),
	ARCH_x64 = (1 << 2),
	ARCH_ANY = ARCH_UNKNOWN | ARCH_x86 | ARCH_x64,
};

struct ProcessInfo_t
{
	DWORD pid;
	EString processName;
	ARCH arch;
};

class CProcessSnapshot
{
private:
	typedef std::vector<ProcessInfo_t> ProcessList;
	typedef std::shared_ptr<const ProcessList> sharedProcessSnap;

	std::atomic<sharedProcessSnap> current;

public:

	void publish(ProcessList fresh)
	{
		auto snap = std::make_shared<const ProcessList>(std::move(fresh));
		current.store(std::move(snap), std::memory_order_release);
	}

	sharedProcessSnap latest() const
	{
		return current.load(std::memory_order_acquire);
	}
};

class CProcessManager
{
private:
	static inline CProcessSnapshot processSnapshot;

public:
	static void UpdateProcessList();
	static std::shared_ptr<const std::vector<ProcessInfo_t>> GetProcessList(const std::string& filterByName = {}, int filterByArchitecture = ARCH_ANY);
	static std::optional<ProcessInfo_t> GetProcessByPid(DWORD pid);
};

