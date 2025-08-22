//
// Created by Gianni on 22/08/2025.
//

#include "camera.hpp"

Camera::Camera(vec3 position, float fovyDegrees, float w, float h, float zNear, float zFar)
    : mPosition(position)
    , mFront(0.f, 0.f, 1.f)
    , mUp(0.f, 1.f, 0.f)
    , mRight(1.f, 0.f, 0.f)
    , mThetaDeg()
    , mPhiDeg()
    , mFovyDeg(fovyDegrees)
    , mNearZ(zNear)
    , mFarZ(zFar)
    , mView(glm::identity<mat4>())
    , mProjection(glm::perspective(glm::radians(fovyDegrees), w / h, zNear, zFar))
    , mViewProj(mProjection * mView)
{
}

void Camera::update(float dt)
{
    if (GetAsyncKeyState('W') & 0x8000) {mPosition += mFront * dt;}
    if (GetAsyncKeyState('S') & 0x8000) {mPosition -= mFront * dt;}
    if (GetAsyncKeyState('A') & 0x8000) {mPosition -= mRight * dt;}
    if (GetAsyncKeyState('D') & 0x8000) {mPosition += mRight * dt;}

    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(GetForegroundWindow(), &mousePos);

    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
    {
        float dx = static_cast<float>(mousePos.x) - mMouseX;
        float dy = static_cast<float>(mousePos.y) - mMouseY;

        mThetaDeg += dx;
        mPhiDeg += dy;
        mPhiDeg = glm::clamp(mPhiDeg, -89.f, 89.f);

        float thetaRad = glm::radians(mThetaDeg);
        float phiRad = glm::radians(mPhiDeg);

        mFront.x = glm::sin(thetaRad) * glm::cos(phiRad);
        mFront.y = glm::sin(phiRad);
        mFront.z = glm::cos(thetaRad) * glm::cos(phiRad);

        mRight = glm::normalize(glm::cross(glm::vec3(0.f, 1.f, 0.f), mFront));
        mUp = glm::normalize(glm::cross(mFront, mRight));
    }

    mMouseX = static_cast<float>(mousePos.x);
    mMouseY = static_cast<float>(mousePos.y);

    mView = glm::lookAt(mPosition, mFront, mUp);
    mViewProj = mProjection * mView;
}

void Camera::resize(float w, float h)
{
    mProjection = glm::perspective(glm::radians(mFovyDeg), w / h, mNearZ, mFarZ);
}

const vec3 &Camera::position() const
{
    return mPosition;
}

const mat4 &Camera::view() const
{
    return mView;
}

const mat4 &Camera::projection() const
{
    return mProjection;
}

const mat4& Camera::viewProj() const
{
    return mViewProj;
}
