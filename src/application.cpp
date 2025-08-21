//
// Created by Gianni on 21/08/2025.
//

#include "application.hpp"

constexpr UINT InitialWindowWidth = 1200;
constexpr UINT InitialWindowHeight = 800;

Application::Application(HINSTANCE hInstance)
    : mHinstance(hInstance)
{
    createDebugConsole();
    createWindow();
}

Application::~Application()
{
}

void Application::run()
{
    MSG msg {};

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        update();
        render();
    }
}

void Application::update()
{
}

void Application::render()
{
}

void Application::createDebugConsole()
{
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    std::ios::sync_with_stdio();
}

void Application::createWindow()
{
    WNDCLASSEX windowClass {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = windowProc,
        .hInstance = mHinstance,
        .hCursor = LoadCursor(nullptr, IDC_ARROW),
        .lpszClassName = "D3D12_3D_Animation"
    };

    RegisterClassEx(&windowClass);

    RECT windowRect {
        .left = 0,
        .top = 0,
        .right = InitialWindowWidth,
        .bottom = InitialWindowHeight
    };

    AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, 0);

    mHwnd = CreateWindowEx(0,
                           windowClass.lpszClassName,
                           "D3D12 - 3D Animation",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           InitialWindowWidth,
                           InitialWindowHeight,
                           nullptr,
                           nullptr,
                           mHinstance,
                           this);
    if (!mHwnd)
    {
        DWORD error = GetLastError();
        throw std::runtime_error("Windows error code: " + std::to_string(error));
    }

    ShowWindow(mHwnd, SW_SHOW);
}

LRESULT CALLBACK Application::windowProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            return 0;
    }

    return DefWindowProc(hwnd, msg, w, l);
}
