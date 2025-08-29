//
// Created by Gianni on 25/08/2025.
//

#include "utils.hpp"

static std::function<void()> dx12DebugCallback;

void check(HRESULT hr, const std::string &msg)
{
    if (FAILED(hr))
    {
        dx12DebugCallback();
        throw std::runtime_error(msg);
    }
}

void setD3D12DebugCallback(std::function<void()> callback)
{
    dx12DebugCallback = callback;
}

ID3D12GraphicsCommandList* beginSingleTimeCommands(ID3D12Device* device, ID3D12CommandAllocator* cmdAllocator)
{
    ID3D12GraphicsCommandList* commandList;
    auto hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&commandList));
    check(hr, "Failed to create command list.");
    return commandList;
}

void endSingleTimeCommands(ID3D12Device* device,
                           ID3D12GraphicsCommandList* cmdList,
                           ID3D12CommandQueue* queue)
{
    ID3D12CommandList* ppCmdLists[] = { cmdList };
    queue->ExecuteCommandLists(1, ppCmdLists);

    ID3D12Fence* fence;
    auto hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    check(hr, "Failed to create fence.");

    hr = queue->Signal(fence, 1);
    check(hr, "Failed to signal fence.");

    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    fence->SetEventOnCompletion(1, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);

    fence->Release();
    cmdList->Release();
}

void waitDeviceIdle(ID3D12Device* device, ID3D12CommandQueue* queue)
{
    ID3D12Fence* fence;
    auto hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    check(hr, "Failed to create fence.");

    HANDLE event = CreateEvent(nullptr, 0, 0, nullptr);

    hr = queue->Signal(fence, 1);
    check(hr, "Failed to signal fence.");

    if (fence->GetCompletedValue() < 1)
    {
        fence->SetEventOnCompletion(1, event);
        WaitForSingleObject(event, INFINITE);
    }

    CloseHandle(event);
    fence->Release();
}
