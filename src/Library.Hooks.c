#define INITGUID
#define _MINAPPMODEL_H_
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS

#include <windows.h>
#include <dxgi1_6.h>
#include <d3d11_4.h>
#include <minhook.h>

PBYTE __wrap_memcpy(PBYTE destination, PBYTE source, SIZE_T count)
{
    __movsb(destination, source, count);
    return destination;
}

PBYTE __wrap_memset(PBYTE destination, BYTE data, SIZE_T count)
{
    __stosb(destination, data, count);
    return destination;
}

HWND Window = NULL;

INT (*_ShowCursor)(BOOL show) = NULL;

ATOM (*_RegisterClassExW)(PWNDCLASSEXW value) = NULL;

HRESULT (*_Present)(IDXGISwapChain4 *swapchain, UINT interval, UINT flags) = NULL;

HRESULT (*_ResizeBuffers)(IDXGISwapChain4 *swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format,
                          UINT flags) = NULL;

HRESULT (*_ResizeBuffers1)(IDXGISwapChain4 *swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format,
                           UINT flags, PUINT mask, LPUNKNOWN queue) = NULL;

HRESULT (*_CreateSwapChain)(IDXGIFactory7 *factory, IUnknown *device, DXGI_SWAP_CHAIN_DESC *description,
                            IDXGISwapChain4 **swapchain) = NULL;

HRESULT $Present(IDXGISwapChain4 *swapchain, UINT interval, UINT flags)
{
    if (!interval && !(flags & DXGI_PRESENT_ALLOW_TEARING))
        flags |= DXGI_PRESENT_ALLOW_TEARING;
    return _Present(swapchain, interval, flags);
}

HRESULT $ResizeBuffers(IDXGISwapChain4 *swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
{
    if (!(flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING))
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    return _ResizeBuffers(swapchain, count, width, height, format, flags);
}

HRESULT $ResizeBuffers1(IDXGISwapChain4 *swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format, UINT flags,
                        PUINT mask, LPUNKNOWN queue)
{
    if (!(flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING))
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    return _ResizeBuffers1(swapchain, count, width, height, format, flags, mask, queue);
}

HRESULT $CreateSwapChain(IDXGIFactory7 *factory, IUnknown *device, DXGI_SWAP_CHAIN_DESC *description,
                         IDXGISwapChain4 **swapchain)
{
    Window = description->OutputWindow;

    if (!(description->Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING))
        description->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    static BOOL hooked = FALSE;
    HRESULT result = _CreateSwapChain(factory, device, description, swapchain);

    if (SUCCEEDED(result) && !hooked)
    {
        PVOID target = (*swapchain)->lpVtbl->Present;
        MH_CreateHook(target, &$Present, (PVOID *)&_Present);
        MH_QueueEnableHook(target);

        target = (*swapchain)->lpVtbl->ResizeBuffers;
        MH_CreateHook(target, &$ResizeBuffers, (PVOID *)&_ResizeBuffers);
        MH_QueueEnableHook(target);

        target = (*swapchain)->lpVtbl->ResizeBuffers1;
        MH_CreateHook(target, &$ResizeBuffers1, (PVOID *)&_ResizeBuffers1);
        MH_QueueEnableHook(target);

        MH_ApplyQueued();
        hooked = TRUE;
    }

    return result;
}

INT $ShowCursor(BOOL show)
{
    if (!show)
    {
        RECT rectangle = {};
        GetClientRect(Window, &rectangle);

        POINT point = {};
        point.x = (rectangle.right - rectangle.left) / 2;
        point.y = (rectangle.bottom - rectangle.top) / 2;

        ClientToScreen(Window, &point);

        rectangle.left = rectangle.right = point.x;
        rectangle.top = rectangle.bottom = point.y;

        ClipCursor(&rectangle);
    }
    return _ShowCursor(show);
}

HRESULT $D3D12CreateDevice(LPUNKNOWN *adapter, D3D_FEATURE_LEVEL level, REFIID iid, void **device)
{
    return DXGI_ERROR_UNSUPPORTED;
}

ATOM $RegisterClassExW(PWNDCLASSEXW value)
{
    static BOOL hooked = FALSE;

    if (!hooked)
    {
        IDXGIFactory7 *factory = NULL;
        CreateDXGIFactory2(0, &IID_IDXGIFactory7, (PVOID *)&factory);

        PVOID target = factory->lpVtbl->CreateSwapChain;
        MH_CreateHook(target, &$CreateSwapChain, (PVOID *)&_CreateSwapChain);
        MH_QueueEnableHook(target);

        MH_CreateHook(ShowCursor, &$ShowCursor, (PVOID *)&_ShowCursor);
        MH_QueueEnableHook(ShowCursor);

        HMODULE module = LoadLibraryExW(L"D3D12.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        target = GetProcAddress(module, "D3D12CreateDevice");

        MH_CreateHook(target, &$D3D12CreateDevice, NULL);
        MH_QueueEnableHook(target);

        MH_ApplyQueued();
        IDXGIFactory7_Release(factory);

        hooked = TRUE;
    }

    return _RegisterClassExW(value);
}