#include <windows.h>
#include <cstdio>

int main()
{
    char path[MAX_PATH] = { 0 };
    GetModuleFileNameA(nullptr, path, MAX_PATH);

    printf("TargetApp PID   = %lu\n", GetCurrentProcessId());
    printf("TargetApp path  = %s\n", path);
    printf("Waiting for injection... Ctrl+C to exit.\n\n");

    while (true)
    {
        Sleep(1000);
        putchar('.');
        fflush(stdout);
    }
    return 0;
}