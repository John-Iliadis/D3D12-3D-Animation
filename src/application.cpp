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
    setDebugCallback();
    createQueue();
    createCommandAllocator();
    createSwapchain();
    createRtvDescriptorHeap();
    createDsvDescriptorHeap();
    createFrameResources();
    createDepthBuffer();
    createRootSignature();
    createPipeline();
    loadModel();
}

Application::~Application()
{
}

// todo: get dt
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

void Application::setDebugCallback()
{
    ID3D12Device* device = mDevice.Get();
    setD3D12DebugCallback([device]()
    {
        ID3D12InfoQueue *infoQueue;

        if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        {
            const UINT64 numMessages = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();

            for (UINT64 i = 0; i < numMessages; ++i)
            {
                SIZE_T messageLength = 0;
                infoQueue->GetMessage(i, nullptr, &messageLength);

                D3D12_MESSAGE *message = (D3D12_MESSAGE *) malloc(messageLength);

                if (message)
                {
                    infoQueue->GetMessage(i, message, &messageLength);
                    std::cout << "D3D12 Debug: " << message->pDescription << std::endl;
                    free(message);
                }
            }

            infoQueue->ClearStoredMessages();
        }
    });
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

void Application::createDsvDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        .NumDescriptors = 1,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0
    };

    auto hr = mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mDsvDescriptorHeap.GetAddressOf()));
    check(hr, "Failed to create dsv descriptor heap.");
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

void Application::createDepthBuffer()
{
    D3D12_RESOURCE_DESC depthBufferDesc {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = InitialWindowWidth,
        .Height = InitialWindowHeight,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    };

    D3D12_HEAP_PROPERTIES heapProperties {.Type = D3D12_HEAP_TYPE_DEFAULT};

    D3D12_CLEAR_VALUE clearValue{
        .Format = DXGI_FORMAT_D32_FLOAT,
        .DepthStencil = {
            .Depth = 1.f,
            .Stencil = 0
        }
    };

    auto hr = mDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthBufferDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(mDepthBuffer.GetAddressOf()));
    check(hr, "Failed to create depth buffer");

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {
        .Format = DXGI_FORMAT_D32_FLOAT,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE
    };

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    mDevice->CreateDepthStencilView(mDepthBuffer.Get(), &dsvDesc, dsvHandle);
}

void Application::createRootSignature()
{
    D3D12_DESCRIPTOR_RANGE1 descriptorRanges[] {
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .NumDescriptors = 1,
            .BaseShaderRegister = 0,
            .RegisterSpace = 0,
            .OffsetInDescriptorsFromTableStart = 0
        },
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .NumDescriptors = 1,
            .BaseShaderRegister = 1,
            .RegisterSpace = 0,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        },
        {
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = 1,
            .BaseShaderRegister = 0,
            .RegisterSpace = 0,
            .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
        },
    };

    D3D12_ROOT_PARAMETER1 rootParameter {
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
        .DescriptorTable {
            .NumDescriptorRanges = 3,
            .pDescriptorRanges = descriptorRanges
        },
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
    };

    D3D12_STATIC_SAMPLER_DESC staticSamplerDesc {
        .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
    };

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc {
        .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
        .Desc_1_1 = {
            .NumParameters = 1,
            .pParameters = &rootParameter,
            .NumStaticSamplers = 1,
            .pStaticSamplers = &staticSamplerDesc,
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        },
    };

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    auto hr = D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), error.GetAddressOf());
    check(hr, "Failed to serialize versioned root signature.");

    hr = mDevice->CreateRootSignature(0, signature->GetBufferPointer(),
                                      signature->GetBufferSize(),
                                      IID_PPV_ARGS(mRootSignature.GetAddressOf()));
    check(hr, "Failed to create root signature.");
}

void Application::createPipeline()
{
    ComPtr<ID3DBlob> vs = compileShader(L"../shaders/vertex.hlsl", "vs_5_0");
    ComPtr<ID3DBlob> ps = compileShader(L"../shaders/pixel.hlsl", "ps_5_0");

    D3D12_SHADER_BYTECODE vsBytecode {
        .pShaderBytecode = vs->GetBufferPointer(),
        .BytecodeLength = vs->GetBufferSize(),
    };

    D3D12_SHADER_BYTECODE psBytecode {
        .pShaderBytecode = ps->GetBufferPointer(),
        .BytecodeLength = ps->GetBufferSize(),
    };

    D3D12_RENDER_TARGET_BLEND_DESC defaultBlendDesc {
        .BlendEnable = FALSE,
        .LogicOpEnable = FALSE,
        .SrcBlend = D3D12_BLEND_ONE,
        .DestBlend = D3D12_BLEND_ZERO,
        .BlendOp = D3D12_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D12_BLEND_ONE,
        .DestBlendAlpha = D3D12_BLEND_ZERO,
        .BlendOpAlpha = D3D12_BLEND_OP_ADD,
        .LogicOp = D3D12_LOGIC_OP_NOOP,
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
    };

    D3D12_BLEND_DESC blendState {
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = FALSE
    };

    for (auto& n : blendState.RenderTarget)
        n = defaultBlendDesc;

    D3D12_RASTERIZER_DESC rasterDesc {
        .FillMode = D3D12_FILL_MODE_SOLID,
        .CullMode = D3D12_CULL_MODE_NONE,
        .FrontCounterClockwise = FALSE,
        .DepthClipEnable = TRUE,
        .MultisampleEnable = FALSE,
        .AntialiasedLineEnable = FALSE
    };

    D3D12_DEPTH_STENCIL_DESC depthStencilState {
        .DepthEnable = TRUE,
        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
        .StencilEnable = FALSE
    };

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc {
        .pRootSignature = mRootSignature.Get(),
        .VS = vsBytecode,
        .PS = psBytecode,
        .BlendState = blendState,
        .SampleMask = UINT_MAX,
        .RasterizerState = rasterDesc,
        .DepthStencilState = depthStencilState,
        .InputLayout {
            .pInputElementDescs = inputElementDescs,
            .NumElements = 4,
        },
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
        .DSVFormat = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        }
    };

    auto hr = mDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(mPipeline.GetAddressOf()));
    check(hr, "Failed to create pipeline.");
}

void Application::loadModel()
{
    mModel.create("../assets/phoenix_bird/scene.gltf",
                  mDevice, mQueue, mCommandAllocator);
}

void Application::resize()
{

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
