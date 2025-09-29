#include <windows.h>
#include <appmodel.h>
#include <minhook.h>
#include <stdio.h>

EXTERN_C CONST IMAGE_DOS_HEADER __ImageBase;

ATOM $RegisterClassExW(PWNDCLASSEXW value);
EXTERN_C CONST ATOM (*_RegisterClassExW)(PWNDCLASSEXW value);

CONST WCHAR Proxy[] = L"49763184-3E52-4CBB-BF72-82143A5442EE";
CONST WCHAR Library[] = L"FA094946-8D81-4B61-A98E-5A379CF1B02F";

BOOL (*_CreateProcessW)(PCWSTR applicationName, PWSTR commandLine, PSECURITY_ATTRIBUTES processAttributes,
                        PSECURITY_ATTRIBUTES threadAttributes, BOOL inheritHandles, DWORD creationFlags,
                        PVOID environment, PCWSTR currentDirectory, STARTUPINFOW *startupInfo,
                        PPROCESS_INFORMATION processInformation) = NULL;

BOOL $CreateProcessW(PCWSTR applicationName, PWSTR commandLine, PSECURITY_ATTRIBUTES processAttributes,
                     PSECURITY_ATTRIBUTES threadAttributes, BOOL inheritHandles, DWORD creationFlags, PVOID environment,
                     PCWSTR currentDirectory, STARTUPINFOW *startupInfo, PPROCESS_INFORMATION processInformation)
{
    CONST BOOL suspended = creationFlags & CREATE_SUSPENDED;
    if (!suspended)
        creationFlags |= CREATE_SUSPENDED;

    CONST BOOL succeeded =
        _CreateProcessW(applicationName, commandLine, processAttributes, threadAttributes, inheritHandles,
                        creationFlags, environment, currentDirectory, startupInfo, processInformation);

    HANDLE thread = processInformation->hThread;
    HANDLE process = processInformation->hProcess;
    CONST BOOL packaged = GetPackageFamilyName(process, &(UINT32){}, NULL) == ERROR_INSUFFICIENT_BUFFER;

    if (succeeded && packaged)
    {
        WCHAR path[MAX_PATH] = {};
        GetModuleFileNameW((HMODULE)&__ImageBase, path, MAX_PATH);

        if (!GetLastError())
        {
            PVOID address = VirtualAllocEx(process, NULL, sizeof path, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            WriteProcessMemory(process, address, path, sizeof path, NULL);

            SetThreadDescription(thread, Library);
            QueueUserAPC((PAPCFUNC)LoadLibraryW, thread, (ULONG_PTR)address);
        }
    }

    if (!suspended)
        ResumeThread(thread);
    return succeeded;
}

BOOL DllMain(HINSTANCE instance, DWORD reason, PVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        AllocConsole();
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);

        PWSTR description = {};
        DisableThreadLibraryCalls(instance);

        if (SUCCEEDED(GetThreadDescription(GetCurrentThread(), &description)))
        {
            MH_Initialize();

            if (CompareStringOrdinal(Proxy, -1, description, -1, FALSE) == CSTR_EQUAL)
            {
                MH_CreateHook(CreateProcessW, &$CreateProcessW, (PVOID *)&_CreateProcessW);
                MH_EnableHook(CreateProcessW);
            }
            else if (CompareStringOrdinal(Library, -1, description, -1, FALSE) == CSTR_EQUAL)
            {
                MH_CreateHook(RegisterClassExW, &$RegisterClassExW, (PVOID *)&_RegisterClassExW);
                MH_EnableHook(RegisterClassExW);
            }
        }

        LocalFree(description);
    }
    return TRUE;
}