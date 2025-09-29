#define INITGUID
#define _MINAPPMODEL_H_
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS

#include <windows.h>
#include <shobjidl.h>
#include <appmodel.h>
#include <shlwapi.h>

CONST WCHAR Proxy[] = L"49763184-3E52-4CBB-BF72-82143A5442EE";
CONST WCHAR PackageFamilyName[] = L"Microsoft.MinecraftWindowsBeta_8wekyb3d8bbwe";
CONST WCHAR ApplicationUserModelId[] = L"Microsoft.MinecraftWindowsBeta_8wekyb3d8bbwe!Game";

VOID Resume(HANDLE event, PWSTR path)
{
    INT count = 0;
    PWSTR *arguments = CommandLineToArgvW(GetCommandLineW(), &count);

    for (INT index = 0; index < count - 1; index++)
    {
        if (CompareStringOrdinal(L"-tid", -1, arguments[index], -1, FALSE) != CSTR_EQUAL)
            continue;

        HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, StrToIntW(arguments[++index])),
               process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetProcessIdOfThread(thread));

        DWORD size = sizeof(WCHAR) * MAX_PATH;
        PVOID address = VirtualAllocEx(process, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        PathRenameExtensionW(path, L".dll");
        WriteProcessMemory(process, address, path, size, NULL);
        CloseHandle(process);

        SetThreadDescription(thread, Proxy);
        QueueUserAPC((PAPCFUNC)LoadLibraryW, thread, (ULONG_PTR)address);
        
        ResumeThread(thread);
        CloseHandle(thread);
    }

    LocalFree(arguments);
    SetEvent(event);
}

VOID Launch(HANDLE event, PWSTR path)
{
    CoInitialize(NULL);

    IPackageDebugSettings *settings = NULL;
    IApplicationActivationManager *manager = NULL;

    LPIID iid = (LPIID)&IID_IApplicationActivationManager;
    LPCLSID clsid = (LPCLSID)&CLSID_ApplicationActivationManager;
    CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, iid, (PVOID *)&manager);

    iid = (LPIID)&IID_IPackageDebugSettings;
    clsid = (LPCLSID)&CLSID_PackageDebugSettings;
    CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, iid, (PVOID *)&settings);

    UINT32 length = PACKAGE_FULL_NAME_MAX_LENGTH + 1;
    WCHAR name[PACKAGE_FULL_NAME_MAX_LENGTH + 1] = {};
    GetPackagesByPackageFamily(PackageFamilyName, &(UINT32){1}, &(PWSTR){}, &length, name);

    IPackageDebugSettings_DisableDebugging(settings, name);
    IPackageDebugSettings_EnableDebugging(settings, name, path, NULL);

    IApplicationActivationManager_ActivateApplication(manager, ApplicationUserModelId, NULL, AO_NOERRORUI, &(DWORD){});
    IApplicationActivationManager_Release(manager);

    WaitForSingleObject(event, INFINITE);
    IPackageDebugSettings_DisableDebugging(settings, name);
    IPackageDebugSettings_Release(settings);

    CoUninitialize();
}

int main()
{
    WCHAR path[MAX_PATH] = {};
    GetModuleFileNameW(NULL, path, MAX_PATH);

    if (!GetLastError())
    {
        HANDLE event = CreateEventW(NULL, TRUE, FALSE, Proxy);
        (event && GetLastError() ? Resume : Launch)(event, path);
        CloseHandle(event);
    }

    ExitProcess(0);
    return 0;
}