//
// Created by Gianni on 23/08/2025.
//

#include "mesh.hpp"

Mesh::Mesh(const std::vector<Vertex> &vertices,
           const std::vector<UINT> &indices,
           const std::string &texPath,
           ComPtr <ID3D12Device> device,
           ComPtr <ID3D12CommandQueue> queue,
           ComPtr <ID3D12CommandAllocator> cmdAllocator,
           ComPtr <ID3D12DescriptorHeap> descriptorHeap,
           UINT textureIndex)
    : mDevice(device)
    , mIndexCount(indices.size())
{
    createVertexBuffer(vertices);
    createIndexBuffer(indices);
    mBaseColorTexture.create(device, queue, cmdAllocator, texPath);

    D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D {
            .MostDetailedMip = 0,
            .MipLevels = 1,
            .PlaneSlice = 0,
            .ResourceMinLODClamp = 0
        }
    };

    // write descriptor
    UINT descriptorPtr = textureIndex * mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += descriptorPtr;
    mDevice->CreateShaderResourceView(mBaseColorTexture, &shaderResourceViewDesc, srvHandle);

    // get gpu virtual address
    mSrvHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    mSrvHandle.ptr += descriptorPtr;
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

void Mesh::render(ID3D12GraphicsCommandList *cmdList) const
{
    cmdList->SetGraphicsRootDescriptorTable(1, mSrvHandle);
    cmdList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    cmdList->IASetIndexBuffer(&mIndexBufferView);
    cmdList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);
}
