#include <Windows.h>

#include "vk_renderer.h"

bool running = true;

LRESULT CALLBACK platform_window_callback(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res = 0;
    switch (msg)
    {
    case WM_CLOSE:
        running = false;
        break;

    default:
        res = DefWindowProc(wnd, msg, wParam, lParam);
        break;
    }

    return res;
}

bool platform_create_window(HWND* window)
{
    auto instance = GetModuleHandle(0);

    WNDCLASS wc = {};
    wc.lpfnWndProc = platform_window_callback;
    wc.hInstance = instance;
    wc.lpszClassName = L"vulkan_engine_class";
    wc.style = CS_VREDRAW | CS_HREDRAW;

    if (!RegisterClass(&wc))
    {
        MessageBox(*window, L"Failed registering window class", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    *window = CreateWindowEx(WS_EX_APPWINDOW,
        wc.lpszClassName,
        L"Pong",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0,
        instance,
        0);

    if (window == 0)
    {
        MessageBox(*window, L"Failed creating window", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    ShowWindow(*window, SW_SHOW);

    return true;
}

void platform_update_window(HWND& window)
{
    MSG msg = {};

    while (PeekMessage(&msg, window, 0, 0, PM_REMOVE) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int main()
{
    VkContext vkContext = {};

    HWND window = 0;
    if (!platform_create_window(&window))
    {
        return -1;
    }

    if (!vk_init(&vkContext, (void*)&window))
    {
        return -1;
    }

    while (running)
    {
        platform_update_window(window);
    }
}