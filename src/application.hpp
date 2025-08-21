//
// Created by Gianni on 21/08/2025.
//

#ifndef D3D12___3D_ANIMATION_APPLICATION_HPP
#define D3D12___3D_ANIMATION_APPLICATION_HPP

#include <windows.h>
#include <stdexcept>
#include <iostream>

class Application
{
public:
    Application(HINSTANCE hInstance);
    ~Application();

    void run();

private:
    void update();
    void render();

    void createDebugConsole();
    void createWindow();

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);

private:
    HINSTANCE mHinstance;
    HWND mHwnd;
};

#endif //D3D12___3D_ANIMATION_APPLICATION_HPP
