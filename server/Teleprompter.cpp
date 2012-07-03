#include "stdafx.hpp"

#include "Teleprompter.hpp"

Teleprompter* Teleprompter::instance_ = NULL;

void Teleprompter::GlobalInit()
{
    instance_ = new Teleprompter;
}

Teleprompter::Teleprompter()
{
    gui_thread_ = boost::thread(
        &Teleprompter::guiThreadEntryPoint,
        this);
}

Teleprompter::~Teleprompter()
{
    gui_thread_.join();
}

void Teleprompter::guiThreadEntryPoint()
{
    HINSTANCE hInstance = ::GetModuleHandle(NULL);
    char szClassName[] = "OmegaCompleteTeleprompter";

    WNDCLASSEX wc;
    HWND hwnd;
    MSG msg;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        ::MessageBox( 
            NULL,
            "Teleprompter Window Class Registration Failed!",
            "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    hwnd = ::CreateWindowEx(
        WS_EX_CLIENTEDGE,
        szClassName,
        "The title of my window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        240, 120,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hwnd == NULL)
    {
        ::MessageBox(
            NULL,
            "Window Creation Failed!",
            "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    ::ShowWindow(hwnd, SW_SHOW);
    ::UpdateWindow(hwnd);

    while (::GetMessage(&msg, NULL, 0, 0) > 0)
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    return;
}

LRESULT CALLBACK Teleprompter::WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd,msg, wParam, lParam);
        break;
    }

    return 0;
}
