//
// Created by Gianni on 26/08/2025.
//

#ifndef D3D12_3D_ANIMATION_BONE_HPP
#define D3D12_3D_ANIMATION_BONE_HPP

#include <stdexcept>
#include <vector>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

using glm::mat4;
using glm::vec3;
using glm::quat;

struct KeyframePosition
{
    vec3 position;
    float timeStamp;
};

struct KeyframeRotation
{
    quat rotation;
    float timeStamp;
};

struct KeyframeScale
{
    vec3 scale;
    float timeStamp;
};

class Bone
{
public:
    Bone(const std::string& name, int index, const aiNodeAnim* keyframeData);

    void update(float timestamp);

    mat4 getLocalTransform();
    std::string getName();
    int getIndex();

private:
    int getStartingPositionKeyframeIndex(float timestamp);
    int getStartingRotationKeyframeIndex(float timestamp);
    int getStartingScaleKeyframeIndex(float timestamp);
    mat4 interpolatePosition(float timestamp);
    mat4 interpolateRotation(float timestamp);
    mat4 interpolateScale(float timestamp);
    float getInterpolationFactor(float lastTimestamp, float nextTimestamp, float currentTimestamp);

private:
    std::vector<KeyframePosition> mPositions;
    std::vector<KeyframeRotation> mRotations;
    std::vector<KeyframeScale> mScales;
    mat4 mLocalTransform;
    std::string mName;
    int mIndex;
};

#endif //D3D12_3D_ANIMATION_BONE_HPP
