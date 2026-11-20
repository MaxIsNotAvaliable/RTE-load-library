#pragma once
// Credits to TheCruZ
#include <base.h>
#include <iostream>
#include <fstream>
#include <TlHelp32.h>
#include <stdio.h>
#include <tchar.h>
#include <string>

#pragma comment(lib, "psapi.lib")

struct CInjector
{
	static std::string ReturnCodeToStr(int code);

	static bool IsDllInjected(DWORD PID, const char* dllPath);
	static int InjectDllByPid(DWORD PID, const char* dllPath);
	static int UnloadDll(DWORD PID, const char* dllName);
	//static int InjectDllByProcName(const char* processName, const char* dllPath);
};

#include <vector>
struct InjectQueueHandler
{
private:
	enum IQ_TUPLE
	{
		IQH_PID = 0,
		IQH_DLL_PATH = 1,
		IQH_IS_INJECTION_FINISHED = 2,
		IQH_INJECTION_RESULT_CODE = 3,
	};
public:
	/// <summary>
	/// tuple (
	///		pid, dllPath,
	///		isExecuted, result
	/// )
	/// </summary>
	typedef std::tuple<DWORD, std::string, bool, int> queueTuple_t;
	static inline std::vector<queueTuple_t> queueList;

	static bool AddToQueue(DWORD PID, const std::string& dllPath)
	{
		for (size_t i = 0; i < queueList.size(); i++)
		{
			if (std::get<IQH_IS_INJECTION_FINISHED>(queueList[i]) != true)
				return false;
		}

		std::tuple<DWORD, std::string> argTuple(PID, dllPath);
		queueTuple_t obj(PID, dllPath, false, 0);

		queueList.push_back(obj);
		return true;
	}

	static int Execute(int& outResult)
	{
		for (size_t i = 0; i < queueList.size(); i++)
		{
			if (std::get<IQH_IS_INJECTION_FINISHED>(queueList[i]) != true)
			{
				DWORD pid = std::get<IQH_PID>(queueList[i]);
				std::string dllPath = std::get<IQH_DLL_PATH>(queueList[i]);
				outResult = CInjector::InjectDllByPid(pid, dllPath.c_str());
				return i;
			}
		}
		return -1;
	}

	static void SetResult(int id, int result)
	{
		if (id < 0 || id >= queueList.size())
			return;

		queueTuple_t& lastQueue = queueList[id];
		std::get<IQH_IS_INJECTION_FINISHED>(lastQueue) = true;
		std::get<IQH_INJECTION_RESULT_CODE>(lastQueue) = result;
	}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="id"> - Injection id</param>
	/// <param name="pOutCode"> - Contains exit code</param>
	/// <returns> - Is injection finished</returns>
	static bool GetResult(int id, int* pOutCode)
	{
		if (id < 0 || id >= queueList.size())
		{
			*pOutCode = -1;
			return false;
		}

		queueTuple_t& lastQueue = queueList[id];
		bool loaded = std::get<IQH_IS_INJECTION_FINISHED>(lastQueue);
		*pOutCode = loaded ? std::get<IQH_INJECTION_RESULT_CODE>(lastQueue) : -1;
		return loaded;
	}

	static int GetLastId()
	{
		return int(queueList.size()) - 1;
	}
};