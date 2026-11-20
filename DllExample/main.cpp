#include <windows.h>
#include <cstdio>

static void ConsoleLog(const char* msg)
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE || hOut == nullptr)
    {
        // В GUI-процессе консоли нет — создадим свою.
        if (!AllocConsole()) return;
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE || hOut == nullptr) return;
    }

    SYSTEMTIME st; GetLocalTime(&st);
    char line[1024];
    int n = sprintf_s(line,
        "[%02d:%02d:%02d.%03d][pid=%lu] %s\n",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
        GetCurrentProcessId(), msg);
    if (n > 0)
    {
        DWORD written = 0;
        WriteConsoleA(hOut, line, (DWORD)n, &written, nullptr);
    }
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinst);

        char buf[512];
        sprintf_s(buf, "DllMain: loaded into PID=%lu, HINSTANCE=%p",
            GetCurrentProcessId(), (void*)hinst);

        ConsoleLog(buf);
        OutputDebugStringA(buf);          // видно в DebugView/VS Output
        MessageBoxA(nullptr, buf, "TestDll injected", MB_OK | MB_ICONINFORMATION);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        ConsoleLog("DllMain: unloaded");
    }
    return TRUE;
}

extern "C" __declspec(dllexport) int TestFunction()
{
    ConsoleLog("TestFunction() called");
    MessageBoxA(nullptr, "TestFunction() called", "TestDll", MB_OK);
    return 42;
}