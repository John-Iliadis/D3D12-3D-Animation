//
// Created by Gianni on 22/08/2025.
//

#ifndef D3D12_3D_ANIMATION_MODEL_HPP
#define D3D12_3D_ANIMATION_MODEL_HPP

#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <format>
#include <d3d12.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stb/stb_image.h>
#include <array>
#include "mesh.hpp"
#include "bone.hpp"

using glm::mat4;

struct NodeHierarchy
{
    std::string name;
    mat4 transformation;
    std::vector<NodeHierarchy> children;
};

class Model
{
public:
    Model() = default;
    Model(const std::string& path,
          ComPtr<ID3D12Device> device,
          ComPtr<ID3D12CommandQueue> queue,
          ComPtr<ID3D12CommandAllocator> cmdAllocator);

    void create(const std::string& path,
                ComPtr<ID3D12Device> device,
                ComPtr<ID3D12CommandQueue> queue,
                ComPtr<ID3D12CommandAllocator> cmdAllocator);

    void update(float dt);
    void render(ID3D12GraphicsCommandList* cmdList);

private:
    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh, const aiScene* scene);
    void processAnimation(const aiScene* scene);
    void updateAnimation(float dt);
    void updateBones(const NodeHierarchy& node, mat4 parentTransform);

    NodeHierarchy getHierarchy(const aiNode* aiNode);
    std::vector<Vertex> getVertices(aiMesh* mesh);
    std::vector<UINT> getIndices(aiMesh* mesh);

private:
    ComPtr<ID3D12Device> mDevice;
    ComPtr<ID3D12CommandQueue> mQueue;
    ComPtr<ID3D12CommandAllocator> mCmdAllocator;
    std::string mDirectory;
    std::vector<Mesh> mMeshes;
    std::unordered_map<std::string, Bone> mBones;
    NodeHierarchy mRoot;
    std::array<mat4, 100> mBoneMatrices;
    float mAnimDuration;
    int mAnimTicksPerSec;
    float mCurrentTime{};
};

#endif //D3D12_3D_ANIMATION_MODEL_HPP
