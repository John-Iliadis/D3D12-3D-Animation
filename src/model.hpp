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
#include "mesh.hpp"

using glm::mat4;

struct BoneInfo
{
    int index;
    mat4 inverseBindMat;
};

class Model
{
public:
    Model() = default;
    Model(const std::string& path, ID3D12Device* device);
    Model& operator=(Model&& other) noexcept;

private:
    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh, const aiScene* scene);

    std::vector<Vertex> getVertices(aiMesh* mesh);
    std::vector<UINT> getIndices(aiMesh* mesh);

private:
    ID3D12Device* mDevice;
    std::string mDirectory;
    std::unordered_map<std::string, BoneInfo> mBoneInfoMap;
    std::vector<Mesh> mMeshes;
};

#endif //D3D12_3D_ANIMATION_MODEL_HPP
