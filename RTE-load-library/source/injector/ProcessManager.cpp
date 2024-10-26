#include "ProcessManager.h"
#include <tlhelp32.h>

bool IsProcess64Bit(HANDLE hProcess) {
    BOOL isWow64 = FALSE;
    if (IsWow64Process(hProcess, &isWow64)) {
        return !isWow64;
    }
    return false;
}


void CProcessManager::UpdateProcessList()
{
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return;
    }

    std::vector<ProcessInfo_t> processes;
    do {
        ProcessInfo_t pi;
        pi.pid = pe32.th32ProcessID;
        if (pi.pid < 1) continue;
        pi.processName = pe32.szExeFile;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pi.pid);
        if (hProcess) 
        {
            pi.arch = IsProcess64Bit(hProcess) ? ARCH_x64 : ARCH_x86;
            CloseHandle(hProcess);
        }
        else 
        {
            pi.arch = ARCH_UNKNOWN;
        }

        processes.push_back(pi);
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    processList = processes;
}

std::vector<ProcessInfo_t> CProcessManager::GetProcessList(const std::string& filterByName)
{
    if (filterByName[0] == '\0')
        return processList;

    std::vector<ProcessInfo_t> filteredProcesses;

    auto lmFilter = [&](const ProcessInfo_t& ms) 
        { 
            return ms.processName.ToLowerCase().find(filterByName) != std::string::npos || EString(ms.pid).find(filterByName) != EString::npos;
        };

    std::copy_if(processList.begin(), processList.end(), std::back_inserter(filteredProcesses), lmFilter);

	return filteredProcesses;
}

ProcessInfo_t CProcessManager::GetProcessByPid(DWORD pid)
{
    /*size_t left, right, mid = 0;

    left = 0;
    right = processList.size() + 1;
    
    while (left != right)
    {
        mid = (right + left) / 2;
        if (processList[mid].pid > pid)
        {
            right = mid;
        }
        else if (processList[mid].pid < pid)
        {
            left = mid + 1;
        }
        else
        {
            return processList[mid];
        }
    }*/

    for (size_t i = 0; i < processList.size(); i++)
    {
        if (processList[i].pid == pid)
            return processList[i];
    }

    return ProcessInfo_t();
}
