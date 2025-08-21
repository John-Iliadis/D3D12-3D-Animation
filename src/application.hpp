//
// Created by Gianni on 21/08/2025.
//

#ifndef D3D12___3D_ANIMATION_APPLICATION_HPP
#define D3D12___3D_ANIMATION_APPLICATION_HPP

#include <stdexcept>
#include <iostream>
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <d3d12sdklayers.h>
#include <DirectXMath.h>
#include <stb/stb_image.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// todo: Create model class
// todo: Create camera class
// todo: Write shaders

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
    void enableValidation();
    void createFactory();
    void createDevice();
    void createQueue();
    void createCommandAllocator();
    void createSwapchain();
    void createRtvDescriptorHeap();
    void createFrameResources();

    void check(HRESULT hr, const std::string& msg);
    UINT8* loadImage(const char* path, int* width, int* height);
    ComPtr<ID3DBlob> compileShader(const wchar_t* path, const char* target);

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);

private:
    HINSTANCE mHinstance;
    HWND mHwnd;

    ComPtr<IDXGIFactory4> mFactory;
    ComPtr<ID3D12Device> mDevice;
    ComPtr<ID3D12CommandQueue> mQueue;
    ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    ComPtr<IDXGISwapChain3> mSwapchain;
    ComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap;
    ComPtr<ID3D12Resource> mRenderTargets[2];

    UINT mFrameIndex;
    UINT mRtvDescriptorSize;
};

#endif //D3D12___3D_ANIMATION_APPLICATION_HPP
