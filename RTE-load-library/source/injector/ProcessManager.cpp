#include "ProcessManager.h"
#include <tlhelp32.h>

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
        pi.processName = pe32.szExeFile;
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
