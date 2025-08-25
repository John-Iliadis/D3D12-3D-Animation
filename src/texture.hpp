//
// Created by Gianni on 23/08/2025.
//

#ifndef D3D12_3D_ANIMATION_TEXTURE_HPP
#define D3D12_3D_ANIMATION_TEXTURE_HPP

#include <string>
#include <stdexcept>
#include <wrl.h>
#include <d3d12.h>
#include <stb/stb_image.h>
#include <glm/glm.hpp>
#include <d3dx12/d3dx12_resource_helpers.h>
#include "utils.hpp"

using Microsoft::WRL::ComPtr;

class Texture
{
public:
    Texture() = default;
    Texture(ComPtr<ID3D12Device> device,
            ComPtr<ID3D12CommandQueue> queue,
            ComPtr<ID3D12CommandAllocator> cmdAllocator,
            const std::string& path);
    void create(ComPtr<ID3D12Device> device,
                ComPtr<ID3D12CommandQueue> queue,
                ComPtr<ID3D12CommandAllocator> cmdAllocator,
                const std::string& path);
    operator ID3D12Resource*() const;

private:
    void createTextureResource();
    ComPtr<ID3D12Resource> createStagingBuffer();
    void uploadTextureData(ComPtr<ID3D12Resource> stagingBuffer, UINT8* data);

private:
    ComPtr<ID3D12Device> mDevice;
    ComPtr<ID3D12CommandQueue> mQueue;
    ComPtr<ID3D12CommandAllocator> mCmdAllocator;
    ComPtr<ID3D12Resource> mTexture;
    D3D12_RESOURCE_DESC mTexDesc;

    UINT mWidth;
    UINT mHeight;
};

#endif //D3D12_3D_ANIMATION_TEXTURE_HPP
