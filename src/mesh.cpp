//
// Created by Gianni on 23/08/2025.
//

#include "mesh.hpp"

Mesh::Mesh(const std::vector<Vertex> &vertices,
           const std::vector<UINT> &indices,
           const std::string &texPath,
           ComPtr <ID3D12Device> device,
           ComPtr <ID3D12CommandQueue> queue,
           ComPtr <ID3D12CommandAllocator> cmdAllocator)
    : mDevice(device)
{
    createVertexBuffer(vertices);
    createIndexBuffer(indices);
    mBaseColorTexture.create(device, queue, cmdAllocator, texPath);
}

void Mesh::createVertexBuffer(const std::vector<Vertex> &vertices)
{
    UINT bufferSize = sizeof(Vertex) * vertices.size();

    D3D12_HEAP_PROPERTIES heapProperties{.Type = D3D12_HEAP_TYPE_UPLOAD};

    D3D12_RESOURCE_DESC bufferResourceDesc {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = bufferSize,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    };

    auto hr = mDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(mVertexBuffer.GetAddressOf()));
    check(hr, "Failed to create vertex buffer");

    UINT8* writePtr;
    D3D12_RANGE readRange{};
    hr = mVertexBuffer->Map(0, &readRange, (void**) & writePtr);
    check(hr, "Failed to map vertex buffer memory");

    memcpy(writePtr, vertices.data(), bufferSize);
    mVertexBuffer->Unmap(0, nullptr);

    mVertexBufferView = {
        .BufferLocation = mVertexBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = bufferSize,
        .StrideInBytes = sizeof(Vertex)
    };
}

void Mesh::createIndexBuffer(const std::vector<UINT> &indices)
{
    UINT bufferSize = sizeof(UINT) * indices.size();

    D3D12_HEAP_PROPERTIES heapProperties{.Type = D3D12_HEAP_TYPE_UPLOAD};

    D3D12_RESOURCE_DESC bufferResourceDesc {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = bufferSize,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    };

    auto hr = mDevice->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(mIndexBuffer.GetAddressOf()));
    check(hr, "Failed to create vertex buffer");

    UINT8* writePtr;
    D3D12_RANGE readRange{};
    hr = mIndexBuffer->Map(0, &readRange, (void**) & writePtr);
    check(hr, "Failed to map vertex buffer memory");

    memcpy(writePtr, indices.data(), bufferSize);
    mIndexBuffer->Unmap(0, nullptr);

    mIndexBufferView = {
        .BufferLocation = mIndexBuffer->GetGPUVirtualAddress(),
        .SizeInBytes = bufferSize,
        .Format = DXGI_FORMAT_R32_UINT
    };
}
