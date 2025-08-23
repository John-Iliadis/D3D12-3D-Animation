//
// Created by Gianni on 22/08/2025.
//

#include "model.hpp"

static constexpr uint32_t import_flags
{
    aiProcess_Triangulate |
    aiProcess_RemoveComponent |
    aiProcess_FlipUVs |
    aiProcess_RemoveRedundantMaterials |
    aiProcess_SortByPType
};

static constexpr int remove_components
{
    aiComponent_LIGHTS |
    aiComponent_CAMERAS |
    aiComponent_COLORS |
    aiComponent_NORMALS |
    aiComponent_TANGENTS_AND_BITANGENTS
};

static constexpr int remove_primitives
{
    aiPrimitiveType_POINT |
    aiPrimitiveType_LINE
};

static glm::mat4 aiToGlmMat4(const aiMatrix4x4& aiMat) {
    return glm::mat4(
        aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
        aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
        aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
        aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
    );
}

static uint8_t* loadImage(const char *path, int *width, int *height)
{
    int w;
    int h;
    int channels;

    UINT8* data = stbi_load(path, &w, &h, &channels, 4);

    if (!data)
    {
        throw std::runtime_error("Failed to load image.");
    }

    if (width) *width = w;
    if (height) *height = h;

    return data;
}

Model::Model(const std::string &path, ID3D12Device* device)
    : mDevice(device)
    , mDirectory(path.substr(0), path.find_last_of('/') + 1)
{
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, remove_components);
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, remove_primitives);
    importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);

    const aiScene* scene = importer.ReadFile(path, import_flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    {
        throw std::runtime_error("Failed to load model.");
    }

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    for (uint32_t i = 0; i < node->mNumMeshes; ++i)
    {
        uint32_t meshIndex = node->mMeshes[i];
        aiMesh* mesh = scene->mMeshes[meshIndex];
        processMesh(mesh, scene);
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        processNode(node->mChildren[i], scene);
    }
}

void Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices = getVertices(mesh);
    std::vector<UINT> indices = getIndices(mesh);

    // get bone data
    for (uint32_t i = 0; i < mesh->mNumBones; ++i)
    {
        std::string boneName = mesh->mBones[i]->mName.data;

        // put bone info in map
        if (!mBoneInfoMap.contains(boneName))
        {
            BoneInfo info {
                .index = static_cast<int>(mBoneInfoMap.size()),
                .inverseBindMat = aiToGlmMat4(mesh->mBones[i]->mOffsetMatrix)
            };

            mBoneInfoMap.try_emplace(boneName, info);
        }

        // check which vertices this bone influences
        uint32_t numWeights = mesh->mBones[i]->mNumWeights;
        aiVertexWeight* boneWeights = mesh->mBones[i]->mWeights;

        for (uint32_t w = 0; w < numWeights; ++w)
        {
            uint32_t vertexIndex = boneWeights[w].mVertexId;
            Vertex& vertex = vertices.at(vertexIndex);
            float boneInfluence = boneWeights[w].mWeight;

            for (int v = 0; v < MAX_BONE_INFLUENCES; ++v)
            {
                if (vertex.boneIndices[v] != -1)
                    continue;

                vertex.boneWeights[v] = boneInfluence;
                vertex.boneIndices[v] = mBoneInfoMap.at(boneName).index;
            }
        }
    }

    // todo: load texture
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    aiString baseColorTexName;
    material->GetTexture(aiTextureType_BASE_COLOR, 0, &baseColorTexName);
    std::string baseColorTexPath = std::format("{}/{}", mDirectory, baseColorTexName.C_Str());
}

std::vector<Vertex> Model::getVertices(aiMesh *mesh)
{
    std::vector<Vertex> vertices;
    vertices.reserve(mesh->mNumVertices);

    for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex;

        vertex.position = *reinterpret_cast<glm::vec3*>(&mesh->mVertices[i]);
        vertex.texCoords = *reinterpret_cast<glm::vec3*>(&mesh->mTextureCoords[0][i]);

        vertices.push_back(vertex);
    }

    return vertices;
}

std::vector<UINT> Model::getIndices(aiMesh *mesh)
{
    std::vector<uint32_t> indices;
    indices.reserve(mesh->mNumFaces * 3);

    for (uint32_t i = 0, face_count = mesh->mNumFaces; i < face_count; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
    }

    return indices;
}

Model &Model::operator=(Model &&other) noexcept
{
    if (this != &other)
    {
        mDevice = other.mDevice;
        mDirectory = std::move(other.mDirectory);
        mBoneInfoMap = std::move(other.mBoneInfoMap);
        mMeshes = std::move(other.mMeshes);
    }

    return *this;
}
