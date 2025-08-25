//
// Created by Gianni on 23/08/2025.
//

#ifndef D3D12_3D_ANIMATION_MESH_HPP
#define D3D12_3D_ANIMATION_MESH_HPP

// have own descriptor heaps
// takes in a command list to give out render commands

#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include <glm/glm.hpp>
#include <string>
#include "texture.hpp"

#define MAX_BONE_INFLUENCES 4

struct Vertex
{
    glm::vec3 position;
    glm::vec3 texCoords;
    int boneIndices[MAX_BONE_INFLUENCES];
    float boneWeights[MAX_BONE_INFLUENCES];

    Vertex()
        : position()
        , texCoords()
    {
        for (int i = 0; i < MAX_BONE_INFLUENCES; ++i)
        {
            boneIndices[i] = -1;
            boneWeights[i] = 0.f;
        }
    }
};

using Microsoft::WRL::ComPtr;

// todo: be aware of move constructor
class Mesh
{
public:
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<UINT>& indices,
         const std::string& texPath,
         ComPtr<ID3D12Device> device,
         ComPtr<ID3D12CommandQueue> queue,
         ComPtr<ID3D12CommandAllocator> cmdAllocator);

private:
    void createVertexBuffer(const std::vector<Vertex> &vertices);
    void createIndexBuffer(const std::vector<UINT> &indices);

private:
    ComPtr<ID3D12Device> mDevice;
    ComPtr<ID3D12Resource> mVertexBuffer;
    ComPtr<ID3D12Resource> mIndexBuffer;
    Texture mBaseColorTexture;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
};


#endif //D3D12_3D_ANIMATION_MESH_HPP
