//
// Created by Gianni on 21/08/2025.
//

#include "application.hpp"

Application::Application(HINSTANCE hInstance)
    : mHinstance(hInstance)
    , mWidth(1200)
    , mHeight(800)
    , mCamera(glm::vec3(0.f, 0.f, -1.f), 35, mWidth, mHeight)
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
    createCbvSrvDescriptorHeap();
    createFrameResources();
    createDepthBuffer();
    createMvpBuffer();
    createRootSignature();
    createPipeline();
    loadModel();
}

Application::~Application()
{
}

void Application::run()
{
    MSG msg {};

    using time = std::chrono::high_resolution_clock;
    auto previousTime = time::now();

    while (msg.message != WM_QUIT)
    {
        auto currentTime = time::now();
        auto dt = currentTime - previousTime;
        previousTime = currentTime;

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        update(dt.count());
        render();
    }
}

void Application::update(float dt)
{
    mCamera.update(dt);
    mModel.update(dt);
    updateMVP();
}

void Application::render()
{
    auto hr = mCommandAllocator->Reset();
    check(hr, "Failed to reset command allocator.");

    ComPtr<ID3D12GraphicsCommandList> commandList;
    hr = mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                    mCommandAllocator.Get(),
                                    nullptr, IID_PPV_ARGS(commandList.GetAddressOf()));
    check(hr, "Failed to create command list.");

    populateCommandList(commandList.Get());

    ID3D12CommandList* ppCommandLists[] = {commandList.Get()};
    mQueue->ExecuteCommandLists(1, ppCommandLists);

    hr = mSwapchain->Present(1, 0);
    check(hr, "Failed to present frame.");

    waitDeviceIdle(mDevice.Get(), mQueue.Get());
    mFrameIndex = mSwapchain->GetCurrentBackBufferIndex();
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
        .right = static_cast<long>(mWidth),
        .bottom = static_cast<long>(mHeight)
    };

    AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, 0);

    mHwnd = CreateWindowEx(0,
                           windowClass.lpszClassName,
                           "D3D12 - 3D Animation",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           mWidth,
                           mHeight,
                           nullptr,
                           nullptr,
                           mHinstance,
                           this);
    if (!mHwnd)
    {
        DWORD error = GetLastError();
        throw std::runtime_error("Windows error code: " + std::to_string(error));
    }

    SetWindowLongPtr(mHwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
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
        .Width = mWidth,
        .Height = mHeight,
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
    D3D12_DESCRIPTOR_HEAP_DESC desc {
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

void Application::createCbvSrvDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = 10,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };

    auto hr = mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mCbvSrvDescriptorHeap.GetAddressOf()));
    check(hr, "Failed to create cbv descriptor heap.");
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
        .Width = mWidth,
        .Height = mHeight,
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

    D3D12_CLEAR_VALUE clearValue {
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

void Application::createMvpBuffer()
{
    D3D12_HEAP_PROPERTIES heapProperties{ .Type = D3D12_HEAP_TYPE_UPLOAD };
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(align256(3 * sizeof(mat4)));

    auto hr = mDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(mMvpBuffer.GetAddressOf()));
    check(hr, "Failed to create mvp buffer");

    D3D12_RANGE readRange{};
    mMvpBuffer->Map(0, &readRange, (void**)&mMvpWritePtr);

    mMvpBufferView = {
        .BufferLocation = mMvpBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = align256(UINT(sizeof(mat4) * 3))
    };

    mDevice->CreateConstantBufferView(&mMvpBufferView, mCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void Application::createRootSignature()
{
    D3D12_DESCRIPTOR_RANGE1 descriptorRangeCbv {
        .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
        .NumDescriptors = 2,
        .BaseShaderRegister = 0,
        .RegisterSpace = 0,
        .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
    };

    D3D12_DESCRIPTOR_RANGE1 descriptorRangeSrv {
        .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        .NumDescriptors = 1,
        .BaseShaderRegister = 0,
        .RegisterSpace = 0,
        .OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
    };

    D3D12_ROOT_PARAMETER1 rootParameters[2] {
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable {
                .NumDescriptorRanges = 1,
                .pDescriptorRanges = &descriptorRangeCbv
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
        },
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable {
                .NumDescriptorRanges = 1,
                .pDescriptorRanges = &descriptorRangeSrv
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
        }
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
            .NumParameters = 2,
            .pParameters = rootParameters,
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
                  mDevice, mQueue,
                  mCommandAllocator, mCbvSrvDescriptorHeap);
}

void Application::resize(int w, int h)
{
    if ((w == mWidth && h == mHeight) || !mSwapchain || w == 0 || h == 0)
        return;

    std::cout << "Resized: " << w << ' ' << h << '\n';

    waitDeviceIdle(mDevice.Get(), mQueue.Get());

    mWidth = w;
    mHeight = h;

    mRenderTargets[0]->Release();
    mRenderTargets[1]->Release();
    mDepthBuffer->Release();

    DXGI_SWAP_CHAIN_DESC1 swapchainDesc;
    mSwapchain->GetDesc1(&swapchainDesc);

    auto hr = mSwapchain->ResizeBuffers(2, w, h, swapchainDesc.Format, swapchainDesc.Flags);
    check(hr, "Failed to resize swapchain buffers.");

    createFrameResources();
    createDepthBuffer();
}

void Application::updateMVP()
{
    mat4 model = glm::identity<mat4>();
    mat4 view = mCamera.view();
    mat4 projection = mCamera.projection();

    mat4 mvp[] {model, view, projection};

    memcpy(mMvpWritePtr, mvp, sizeof(mvp));
}

void Application::populateCommandList(ID3D12GraphicsCommandList* commandList)
{
    D3D12_VIEWPORT viewport {
        .TopLeftX = 0.f,
        .TopLeftY = 0.f,
        .Width = static_cast<float>(mWidth),
        .Height = static_cast<float>(mHeight),
        .MinDepth = 0.f,
        .MaxDepth = 1.f
    };

    D3D12_RECT scissor {
        .left = 0,
        .top = 0,
        .right = static_cast<long>(mWidth),
        .bottom = static_cast<long>(mHeight)
    };

    D3D12_RESOURCE_BARRIER resourceBarrier {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource = mRenderTargets[mFrameIndex].Get(),
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
            .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
        }
    };

    commandList->ResourceBarrier(1, &resourceBarrier);

    ID3D12DescriptorHeap** ppHeaps = mCbvSrvDescriptorHeap.GetAddressOf();
    commandList->SetGraphicsRootSignature(mRootSignature.Get());
    commandList->SetDescriptorHeaps(1, ppHeaps);
    commandList->SetGraphicsRootDescriptorTable(0, mCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    const float clearColor[] {0.f, 0.f, 0.f, 1.f};
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += mFrameIndex * mRtvDescriptorSize;

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mModel.render(commandList);

    D3D12_RESOURCE_BARRIER resourceBarrierPresent
    {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource = mRenderTargets[mFrameIndex].Get(),
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
            .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
        }
    };

    commandList->ResourceBarrier(1, &resourceBarrierPresent);
    auto hr = commandList->Close();
    check(hr, "Failed to close command list.");
}

ComPtr<ID3DBlob> Application::compileShader(const wchar_t* path, const char* target)
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
    Application& app = *reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
        {
            int width = LOWORD(l);
            int height = HIWORD(l);
            app.resize(width, height);
            return 0;
        }
    }

    return DefWindowProc(hwnd, msg, w, l);
}
