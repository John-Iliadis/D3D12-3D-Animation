//
// Created by Gianni on 25/08/2025.
//

#ifndef D3D12_3D_ANIMATION_UTILS_HPP
#define D3D12_3D_ANIMATION_UTILS_HPP

#include <string>
#include <stdexcept>
#include <windows.h>
#include <d3d12.h>

void check(HRESULT hr, const std::string& msg);

ID3D12GraphicsCommandList* beginSingleTimeCommands(ID3D12Device* device, ID3D12CommandAllocator* cmdAllocator);
void endSingleTimeCommands(ID3D12Device* device,
                           ID3D12GraphicsCommandList* cmdList,
                           ID3D12CommandQueue* queue);

#endif //D3D12_3D_ANIMATION_UTILS_HPP
