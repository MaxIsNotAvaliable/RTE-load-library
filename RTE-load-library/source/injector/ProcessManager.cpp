#include "ProcessManager.h"
#include <tlhelp32.h>
#include <algorithm>
#include <iterator>
#include <ranges>

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
    processSnapshot.publish(std::move(processes));
    //processList = processes;
}

std::shared_ptr<const std::vector<ProcessInfo_t>> CProcessManager::GetProcessList(const std::string& filterByName, int filterByArchitecture)
{
    //auto lmFilter = [&](const ProcessInfo_t& ms)
    //    {
    //        if ((ms.arch & filterByArchitecture) == 0)
    //            return false;

    //        if (ms.processName.ToLowerCase().find(filterByName) != std::string::npos) // contains part name
    //            return true;

    //        if (EString(ms.pid).find(filterByName) != EString::npos) // contains PID in searchbar
    //            return true;

    //        return false;
    //    };

    auto snap = processSnapshot.latest();
    if (!snap) return nullptr;

    if (filterByName.empty() && filterByArchitecture == ARCH_ANY)
        return snap;

    //const bool hasName = !filterByName.empty();

    std::string needle = filterByName;
    std::transform(needle.begin(), needle.end(), needle.begin(), [](unsigned char c) {return (char)std::tolower(c); });


    auto out = std::make_shared<std::vector<ProcessInfo_t>>();
    out->reserve(snap->size() / 2 + 1);

    for (const auto& p : *snap)
    {
        if ((p.arch & filterByArchitecture) == 0) continue;

        if (p.processName.ToLowerCase().find(needle) != std::string::npos)
        {
            out->push_back(p);
            continue;
        }
        if (EString(p.pid).find(filterByName) != EString::npos)
        {
            out->push_back(p);
            continue;
        }
    }

    return out;
}

std::optional<ProcessInfo_t> CProcessManager::GetProcessByPid(DWORD pid)
{
    auto snap = processSnapshot.latest();
    if (!snap) return std::nullopt;

    auto it = std::find_if(snap->begin(), snap->end(), [pid](const ProcessInfo_t& p) { return p.pid == pid; });
    if (it == snap->end()) return std::nullopt;
    return *it;

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

    /*for (size_t i = 0; i < processList.size(); i++)
    {
        if (processList[i].pid == pid)
            return processList[i];
    }

    return ProcessInfo_t();*/
}
