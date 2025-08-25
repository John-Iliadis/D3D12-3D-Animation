//
// Created by Gianni on 23/08/2025.
//

#include "texture.hpp"

Texture::Texture(ComPtr<ID3D12Device> device,
                 ComPtr<ID3D12CommandQueue> queue,
                 ComPtr<ID3D12CommandAllocator> cmdAllocator,
                 const std::string& path)
{
    create(device, queue, cmdAllocator, path);
}

void Texture::create(ComPtr<ID3D12Device> device,
                     ComPtr<ID3D12CommandQueue> queue,
                     ComPtr<ID3D12CommandAllocator> cmdAllocator,
                     const std::string& path)
{
    int w, h, c;
    UINT8* data = stbi_load(path.data(), &w, &h, &c, 4);

    if (!data)
    {
        throw std::runtime_error("Failed to load image: " + path);
    }

    mWidth = w;
    mHeight = h;

    mTexDesc =  {
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = static_cast<UINT64>(w),
        .Height = static_cast<UINT>(h),
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN
    };

    createTextureResource();
    auto stagingBuffer = createStagingBuffer();
    uploadTextureData(stagingBuffer, data);
}

void Texture::createTextureResource()
{
    D3D12_HEAP_PROPERTIES textureHeapProperties {.Type = D3D12_HEAP_TYPE_DEFAULT};

    auto hr = mDevice->CreateCommittedResource(&textureHeapProperties,
                                               D3D12_HEAP_FLAG_NONE,
                                               &mTexDesc,
                                               D3D12_RESOURCE_STATE_COPY_DEST,
                                               nullptr,
                                               IID_PPV_ARGS(mTexture.GetAddressOf()));
    check(hr, "Failed to create texture resource.");
}

ComPtr<ID3D12Resource> Texture::createStagingBuffer()
{
    ComPtr<ID3D12Resource> stagingBuffer;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureFootprint;
    UINT64 bufferSize;
    mDevice->GetCopyableFootprints(&mTexDesc,
                                   0, 1, 0,
                                   &textureFootprint,
                                   nullptr,
                                   nullptr,
                                   &bufferSize);

    D3D12_HEAP_PROPERTIES stagingBufferHeapProperties {.Type = D3D12_HEAP_TYPE_UPLOAD};

    D3D12_RESOURCE_DESC stagingBufferDesc {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = bufferSize,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR
    };

    auto hr = mDevice->CreateCommittedResource(&stagingBufferHeapProperties,
                                               D3D12_HEAP_FLAG_NONE,
                                               &stagingBufferDesc,
                                               D3D12_RESOURCE_STATE_GENERIC_READ,
                                               nullptr,
                                               IID_PPV_ARGS(stagingBuffer.GetAddressOf()));
    check(hr, "Failed to create staging buffer.");

    return stagingBuffer;
}

void Texture::uploadTextureData(ComPtr<ID3D12Resource> stagingBuffer, UINT8* data)
{
    const UINT pixelSize = sizeof(UINT);
    D3D12_SUBRESOURCE_DATA textureData {
        .pData = data,
        .RowPitch = mWidth * pixelSize,
        .SlicePitch = mWidth * mHeight * pixelSize
    };

    ID3D12GraphicsCommandList* cmdList = beginSingleTimeCommands(mDevice.Get(), mCmdAllocator.Get());
    UpdateSubresources(cmdList, mTexture.Get(), stagingBuffer.Get(), 0, 0, 1, &textureData);

    D3D12_RESOURCE_BARRIER texTransitionBarrier {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition = {
            .pResource = mTexture.Get(),
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
            .StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        }
    };

    cmdList->ResourceBarrier(1, &texTransitionBarrier);
    cmdList->Close();

    endSingleTimeCommands(mDevice.Get(), cmdList, mQueue.Get());
}

Texture::operator ID3D12Resource*() const
{
    return mTexture.Get();
}
