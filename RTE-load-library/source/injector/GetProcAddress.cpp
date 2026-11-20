#include "GetProcAddress.h"

// ---------------------------------------------------------------------------
// Читает из памяти целевого процесса NT-заголовки и возвращает RVA/Size
// директории экспорта. Корректно работает для PE32 и PE32+ независимо от
// разрядности инжектора — тип заголовка выбирается по OptionalHeader.Magic.
// ---------------------------------------------------------------------------
static bool GetRemoteExportDirectory(HANDLE hProc, BYTE* modBase, DWORD& outRva, DWORD& outSize)
{
    outRva = 0;
    outSize = 0;

    BYTE peHeader[0x1000] = {};
    if (!ReadProcessMemory(hProc, modBase, peHeader, sizeof(peHeader), nullptr))
        return false;

    auto* pDos = reinterpret_cast<IMAGE_DOS_HEADER*>(peHeader);
    if (pDos->e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    BYTE* pNtBase = peHeader + pDos->e_lfanew;
    if (*reinterpret_cast<DWORD*>(pNtBase) != IMAGE_NT_SIGNATURE)
        return false;

    // Magic лежит сразу после Signature(4) + FileHeader(20).
    WORD magic = *reinterpret_cast<WORD*>(pNtBase + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));

    if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        auto* pNt = reinterpret_cast<IMAGE_NT_HEADERS32*>(pNtBase);
        auto& dir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        outRva = dir.VirtualAddress;
        outSize = dir.Size;
        return true;
    }
    if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        auto* pNt = reinterpret_cast<IMAGE_NT_HEADERS64*>(pNtBase);
        auto& dir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        outRva = dir.VirtualAddress;
        outSize = dir.Size;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Поиск базового адреса модуля в целевом процессе.
//
// Для WOW64-процесса, когда инжектор собран под x64, обязательно нужен
// флаг TH32CS_SNAPMODULE32 — иначе снапшот вернёт 64-битное представление
// (ntdll, wow64*.dll) без 32-битной kernel32.dll.
// ---------------------------------------------------------------------------
HINSTANCE GetModuleHandleEx(HANDLE hTargetProc, const TCHAR* lpModuleName)
{
    if (!hTargetProc || !lpModuleName)
        return NULL;

    DWORD pid = GetProcessId(hTargetProc);
    if (!pid)
        return NULL;

    DWORD flags = TH32CS_SNAPMODULE;

#ifdef _WIN64
    BOOL wow = FALSE;
    if (IsWow64Process(hTargetProc, &wow) && wow)
        flags |= TH32CS_SNAPMODULE32;
#endif

    MODULEENTRY32 ME32{};
    ME32.dwSize = sizeof(ME32);

    HANDLE hSnap = CreateToolhelp32Snapshot(flags, pid);
    while (hSnap == INVALID_HANDLE_VALUE && GetLastError() == ERROR_BAD_LENGTH)
    {
        hSnap = CreateToolhelp32Snapshot(flags, pid);
    }
    if (hSnap == INVALID_HANDLE_VALUE)
        return NULL;

    BOOL bRet = Module32First(hSnap, &ME32);
    while (bRet)
    {
        if (!_tcsicmp(lpModuleName, ME32.szModule))
            break;
        bRet = Module32Next(hSnap, &ME32);
    }
    CloseHandle(hSnap);

    return bRet ? ME32.hModule : NULL;
}

// ---------------------------------------------------------------------------
// Резолвинг экспорта в адресном пространстве целевого процесса.
//
// Работает для любой комбинации: x86->x86, x64->x64, x64->x86 (WOW64).
// Вся PE-логика опирается на RVA, которые одинаковы для обеих разрядностей;
// различие только в выборе IMAGE_NT_HEADERS32/64 по OptionalHeader.Magic.
// ---------------------------------------------------------------------------
void* GetProcAddressEx(HANDLE hTargetProc, const TCHAR* lpModuleName, const char* lpProcName)
{
    if (!hTargetProc || !lpModuleName || !lpProcName)
        return nullptr;

    auto* modBase = reinterpret_cast<BYTE*>(GetModuleHandleEx(hTargetProc, lpModuleName));
    if (!modBase)
        return nullptr;

    DWORD expRva = 0, expSize = 0;
    if (!GetRemoteExportDirectory(hTargetProc, modBase, expRva, expSize) || !expSize)
        return nullptr;

    std::vector<BYTE> export_data(expSize);
    if (!ReadProcessMemory(hTargetProc, modBase + expRva,
        export_data.data(), expSize, nullptr))
        return nullptr;

    // localBase — виртуальный «нуль» образа, чтобы к RVA прибавлять напрямую.
    BYTE* localBase = export_data.data() - expRva;
    auto* pExportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(export_data.data());

    auto Forward = [&](DWORD FuncRVA) -> void*
        {
            char pFullExport[MAX_PATH + 1] = {};
            const char* src = reinterpret_cast<const char*>(localBase + FuncRVA);
            size_t len = strnlen(src, MAX_PATH);
            if (!len) return nullptr;
            memcpy(pFullExport, src, len);
            pFullExport[len] = 0;

            char* pFuncName = strchr(pFullExport, '.');
            if (!pFuncName) return nullptr;
            *(pFuncName++) = 0;

            if (*pFuncName == '#')
                pFuncName = reinterpret_cast<char*>(LOWORD(atoi(++pFuncName)));

#ifdef UNICODE
            TCHAR ModNameW[MAX_PATH + 1] = {};
            size_t SizeOut = 0;
            mbstowcs_s(&SizeOut, ModNameW, pFullExport, MAX_PATH);
            return GetProcAddressEx(hTargetProc, ModNameW, pFuncName);
#else
            return GetProcAddressEx(hTargetProc, pFullExport, pFuncName);
#endif
        };

    // --- путь 1: аргумент задан как ординал (значение <= MAXWORD) ---
    if ((reinterpret_cast<UINT_PTR>(lpProcName) & 0xFFFFFF) <= MAXWORD)
    {
        WORD Base = LOWORD(pExportDir->Base - 1);
        WORD Ordinal = LOWORD(reinterpret_cast<UINT_PTR>(lpProcName)) - Base;

        if (Ordinal >= pExportDir->NumberOfFunctions)
            return nullptr;

        DWORD FuncRVA = reinterpret_cast<DWORD*>(localBase + pExportDir->AddressOfFunctions)[Ordinal];

        if (FuncRVA >= expRva && FuncRVA < expRva + expSize)
            return Forward(FuncRVA);

        return modBase + FuncRVA;
    }

    // --- путь 2: поиск по имени (имена отсортированы, бинарный поиск) ---
    if (!pExportDir->NumberOfNames)
        return nullptr;

    DWORD max = pExportDir->NumberOfNames - 1;
    DWORD min = 0;
    DWORD FuncRVA = 0;

    auto* pNames = reinterpret_cast<DWORD*>(localBase + pExportDir->AddressOfNames);
    auto* pOrds = reinterpret_cast<WORD*>(localBase + pExportDir->AddressOfNameOrdinals);
    auto* pFuncs = reinterpret_cast<DWORD*>(localBase + pExportDir->AddressOfFunctions);

    while (min <= max)
    {
        DWORD mid = (min + max) / 2;
        const char* szName = reinterpret_cast<const char*>(localBase + pNames[mid]);
        int cmp = strcmp(szName, lpProcName);

        if (cmp < 0)       min = mid + 1;
        else if (cmp > 0)  max = mid - 1;
        else
        {
            WORD Ordinal = pOrds[mid];
            FuncRVA = pFuncs[Ordinal];
            break;
        }
    }

    if (!FuncRVA)
        return nullptr;

    if (FuncRVA >= expRva && FuncRVA < expRva + expSize)
        return Forward(FuncRVA);

    return modBase + FuncRVA;
}

// ---------------------------------------------------------------------------
// Список всех имён экспортов модуля (в порядке таблицы имён).
// Тот же фикс cross-bitness: заголовок выбирается по OptionalHeader.Magic.
// ---------------------------------------------------------------------------
std::vector<std::string> GetAllFunctions(HANDLE hTargetProc, const TCHAR* lpModuleName)
{
    std::vector<std::string> functionList;

    if (!hTargetProc || !lpModuleName)
        return functionList;

    auto* modBase = reinterpret_cast<BYTE*>(GetModuleHandleEx(hTargetProc, lpModuleName));
    if (!modBase)
        return functionList;

    DWORD expRva = 0, expSize = 0;
    if (!GetRemoteExportDirectory(hTargetProc, modBase, expRva, expSize) || !expSize)
        return functionList;

    std::vector<BYTE> export_data(expSize);
    if (!ReadProcessMemory(hTargetProc, modBase + expRva,
        export_data.data(), expSize, nullptr))
        return functionList;

    BYTE* localBase = export_data.data() - expRva;
    auto* pExportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(export_data.data());

    if (!pExportDir->NumberOfNames)
        return functionList;

    auto* pNames = reinterpret_cast<DWORD*>(localBase + pExportDir->AddressOfNames);

    functionList.reserve(pExportDir->NumberOfNames);
    for (DWORD i = 0; i < pExportDir->NumberOfNames; ++i)
    {
        const char* szName = reinterpret_cast<const char*>(localBase + pNames[i]);
        if (szName && *szName)
            functionList.emplace_back(szName);
    }

    return functionList;
}