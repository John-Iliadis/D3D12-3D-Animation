//
// Created by Gianni on 21/08/2025.
//

#include "application.hpp"

constexpr UINT InitialWindowWidth = 1200;
constexpr UINT InitialWindowHeight = 800;

Application::Application(HINSTANCE hInstance)
    : mHinstance(hInstance)
    , mCamera(glm::vec3(0.f, 0.f, -1.f), 35, InitialWindowWidth, InitialWindowHeight)
{
    createDebugConsole();
    createWindow();
    enableValidation();
    createFactory();
    createDevice();
    createQueue();
    createCommandAllocator();
    createSwapchain();
    createRtvDescriptorHeap();
    createFrameResources();

    loadModel();
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

void Application::enableValidation()
{
    ComPtr<ID3D12Debug> debug;
    auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()));
    check(hr, "Failed to get debug interface.");
    debug->EnableDebugLayer();
}

void Application::createFactory()
{
    UINT factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
    auto hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(mFactory.GetAddressOf()));
    check(hr, "Failed to create factory.");
}

void Application::createDevice()
{
    ComPtr<IDXGIAdapter1> adapter;

    for (UINT adapterIndex = 0; mFactory->EnumAdapters1(adapterIndex, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 adapterDesc;
        adapter->GetDesc1(&adapterDesc);

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice))))
        {
            std::wcout << L"Selected device: " << adapterDesc.Description << '\n';
            return;
        }
    }

    throw std::runtime_error("Failed to create device.");
}

void Application::createQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc{
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
    };

    auto hr = mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(mQueue.GetAddressOf()));
    check(hr, "Failed to create command queue.");
}

void Application::createCommandAllocator()
{
    auto hr = mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                              IID_PPV_ARGS(mCommandAllocator.GetAddressOf()));
    check(hr, "Failed to create command allocator.");
}

void Application::createSwapchain()
{
    ComPtr<IDXGISwapChain1> swapChain1;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{
        .Width = InitialWindowWidth,
        .Height = InitialWindowHeight,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 2,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
    };

    auto hr = mFactory->CreateSwapChainForHwnd(mQueue.Get(),
                                               mHwnd,
                                               &swapChainDesc,
                                               nullptr,
                                               nullptr,
                                               swapChain1.GetAddressOf());
    check(hr, "Failed to create swapchain.");

    mFactory->MakeWindowAssociation(mHwnd, DXGI_MWA_NO_ALT_ENTER);

    swapChain1.As(&mSwapchain);
    mFrameIndex = mSwapchain->GetCurrentBackBufferIndex();
}

void Application::createRtvDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc{
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = 2,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0
    };

    auto hr = mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mRtvDescriptorHeap.GetAddressOf()));
    check(hr, "Failed to create rtv descriptor heap.");

    mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void Application::createFrameResources()
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < 2; ++i)
    {
        auto hr = mSwapchain->GetBuffer(i, IID_PPV_ARGS(mRenderTargets[i].GetAddressOf()));
        check(hr, "Failed to get swapchain buffer");
        mDevice->CreateRenderTargetView(mRenderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += mRtvDescriptorSize;
    }
}

void Application::loadModel()
{
    mModel = Model("../assets/phoenix_bird/scene.gltf", mDevice.Get());
}

void Application::check(HRESULT hr, const std::string &msg)
{
    if (FAILED(hr))
    {
        throw std::runtime_error(msg);
    }
}

ComPtr<ID3DBlob> Application::compileShader(const wchar_t *path, const char *target)
{
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    ComPtr<ID3DBlob> bytebode;
    ComPtr<ID3DBlob> errors;

    auto hr = D3DCompileFromFile(
        path,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        target,
        compileFlags, 0,
        bytebode.GetAddressOf(),
        errors.GetAddressOf());

    if (errors)
    {
        std::cout << (char*)errors->GetBufferPointer() << std::endl;
    }

    check(hr, "Failed to compile shader.");

    return bytebode;
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
