#pragma once
#include <base.h>
#include <future>

struct InjectTarget
{
	static inline DWORD pid;
	static inline char filePath[MAX_PATH] = { '\0' };
};

//class Injection
//{
//private:
//	static inline std::string m_dllFilename;
//	static inline uint32_t m_pid = 0;
//
//public:
//	static void Event_SetPid(uint32_t pid)
//	{
//		m_pid = pid;
//	}
//
//	static void Event_SetDllFilename(const std::string& filename)
//	{
//		m_dllFilename = filename;
//	}
//};