//
// Created by Gianni on 26/08/2025.
//

#include "bone.hpp"

Bone::Bone(const std::string &name, int index, const aiNodeAnim *keyframeData)
    : mName(name)
    , mIndex(index)
{
    // get keyframe positions
    for (uint32_t i = 0; i < keyframeData->mNumPositionKeys; ++i)
    {
        KeyframePosition keyframePosition {
            .position = *reinterpret_cast<vec3*>(&keyframeData->mPositionKeys[i].mValue),
            .timeStamp = static_cast<float>(keyframeData->mPositionKeys[i].mTime)
        };

        mPositions.push_back(keyframePosition);
    }

    // get keyframe rotations
    for (uint32_t i = 0; i < keyframeData->mNumRotationKeys; ++i)
    {
        auto aiQuat = keyframeData->mRotationKeys[i].mValue;
        KeyframeRotation keyframeRotation {
            .rotation = quat(aiQuat.w, aiQuat.x, aiQuat.y, aiQuat.z),
            .timeStamp = static_cast<float>(keyframeData->mRotationKeys[i].mTime)
        };

        mRotations.push_back(keyframeRotation);
    }

    // get keyframe scales
    for (uint32_t i = 0; i < keyframeData->mNumScalingKeys; ++i)
    {
        KeyframeScale keyframeScale {
            .scale = *reinterpret_cast<vec3*>(&keyframeData->mScalingKeys[i].mValue),
            .timeStamp = static_cast<float>(keyframeData->mScalingKeys[i].mTime)
        };

        mScales.push_back(keyframeScale);
    }
}

void Bone::update(float timestamp)
{
    mat4 translation = interpolatePosition(timestamp);
    mat4 rotation = interpolateRotation(timestamp);
    mat4 scale = interpolateScale(timestamp);
    mLocalTransform = translation * rotation * scale;
}

mat4 Bone::getLocalTransform()
{
    return mLocalTransform;
}

std::string Bone::getName()
{
    return mName;
}

int Bone::getIndex()
{
    return mIndex;
}

int Bone::getStartingPositionKeyframeIndex(float timestamp)
{
    for (int i = 0; i < mPositions.size() - 1; ++i)
        if (timestamp < mPositions.at(i + 1).timeStamp)
            return i;
    throw std::runtime_error("End of function reached");
}

int Bone::getStartingRotationKeyframeIndex(float timestamp)
{
    for (int i = 0; i < mRotations.size() - 1; ++i)
        if (timestamp < mRotations.at(i + 1).timeStamp)
            return i;
    throw std::runtime_error("End of function reached");
}

int Bone::getStartingScaleKeyframeIndex(float timestamp)
{
    for (int i = 0; i < mScales.size() - 1; ++i)
        if (timestamp < mScales.at(i + 1).timeStamp)
            return i;
    throw std::runtime_error("End of function reached");
}

mat4 Bone::interpolatePosition(float timestamp)
{
    if (mPositions.size() == 1)
    {
        return glm::translate(glm::identity<mat4>(), mPositions.front().position);
    }

    int frameIndex1 = getStartingPositionKeyframeIndex(timestamp);
    int frameIndex2 = frameIndex1 + 1;

    float interpolationFactor = getInterpolationFactor(mPositions.at(frameIndex1).timeStamp,
                                                       mPositions.at(frameIndex2).timeStamp,
                                                       timestamp);

    vec3 translation = glm::mix(mPositions.at(frameIndex1).position,
                                mPositions.at(frameIndex2).position,
                                interpolationFactor);

    return glm::translate(glm::identity<mat4>(), translation);
}

mat4 Bone::interpolateRotation(float timestamp)
{
    if (mRotations.size() == 1)
    {
        return glm::toMat4(glm::normalize(mRotations.front().rotation));
    }

    int frameIndex1 = getStartingRotationKeyframeIndex(timestamp);
    int frameIndex2 = frameIndex1 + 1;

    float interpolationFactor = getInterpolationFactor(mRotations.at(frameIndex1).timeStamp,
                                                       mRotations.at(frameIndex2).timeStamp,
                                                       timestamp);

    quat rotation = glm::slerp(mRotations.at(frameIndex1).rotation,
                               mRotations.at(frameIndex2).rotation,
                               interpolationFactor);

    return glm::toMat4(glm::normalize(rotation));
}

mat4 Bone::interpolateScale(float timestamp)
{
    if (mScales.size() == 1)
    {
        return glm::scale(glm::identity<mat4>(), mScales.front().scale);
    }

    int frameIndex1 = getStartingScaleKeyframeIndex(timestamp);
    int frameIndex2 = frameIndex1 + 1;

    float interpolationFactor = getInterpolationFactor(mScales.at(frameIndex1).timeStamp,
                                                       mScales.at(frameIndex2).timeStamp,
                                                       timestamp);

    vec3 scale = glm::mix(mScales.at(frameIndex1).scale,
                          mScales.at(frameIndex2).scale,
                          interpolationFactor);

    return glm::scale(glm::identity<mat4>(), scale);
}

float Bone::getInterpolationFactor(float lastTimestamp, float nextTimestamp, float currentTimestamp)
{
    float midWayLength = currentTimestamp - lastTimestamp;
    float framesDiff = nextTimestamp - lastTimestamp;
    return  midWayLength / framesDiff;
}
