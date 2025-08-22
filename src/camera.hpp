//
// Created by Gianni on 22/08/2025.
//

#ifndef D3D12_3D_ANIMATION_CAMERA_HPP
#define D3D12_3D_ANIMATION_CAMERA_HPP

#include <windows.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using glm::vec3;
using glm::mat4;

class Camera
{
public:
    Camera() = default;
    Camera(vec3 position, float fovyDegrees, float w, float h, float zNear = 0.1f, float zFar = 100.f);

    void update(float dt);
    void resize(float w, float h);

    const vec3& position() const;
    const mat4& view() const;
    const mat4& projection() const;
    const mat4& viewProj() const;

private:
    vec3 mPosition;
    vec3 mFront;
    vec3 mUp;
    vec3 mRight;
    float mThetaDeg;
    float mPhiDeg;
    float mFovyDeg;
    float mNearZ;
    float mFarZ;

    mat4 mView;
    mat4 mProjection;
    mat4 mViewProj;

    float mMouseX;
    float mMouseY;
};

#endif //D3D12_3D_ANIMATION_CAMERA_HPP
