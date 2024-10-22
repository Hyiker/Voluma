#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Core/Math.h"
#include "Program/ShaderVar.h"
#include "Utils/UiInputs.h"

namespace Voluma {

static_assert(sizeof(CameraData) % (sizeof(float4)) == 0,
              "CameraData size should be a multiple of 16");
OrbitController::OrbitController(Camera* pCamera) : mpCamera(pCamera) {
    mCameraDistance =
        glm::length(mpCamera->mData.posW - mpCamera->mData.target);
}

float2 convertCamPosRange(const float2 pos) {
    const float2 scale(-2, -2);
    const float2 offset(1, 1);
    float2 res = (pos * scale) + offset;
    return res;
}
void OrbitController::onMouseEvent(const MouseEvent& mouseEvent) {
    switch (mouseEvent.type) {
        case MouseEvent::Type::Wheel: {
            mCameraDistance -= (mouseEvent.wheelDelta.y * 0.2f);
        } break;
        case MouseEvent::Type::ButtonDown:
            if (mouseEvent.button == Input::MouseButton::Left) {
                mLastVector = project2DCrdToUnitSphere(
                    convertCamPosRange(mouseEvent.pos));
                mIsLeftButtonDown = true;
            }
            break;
        case MouseEvent::Type::ButtonUp:
            if (mouseEvent.button == Input::MouseButton::Left) {
                mIsLeftButtonDown = false;
            }
            break;
        case MouseEvent::Type::Move:
            if (mIsLeftButtonDown) {
                float3 curVec = project2DCrdToUnitSphere(
                    convertCamPosRange(mouseEvent.pos));
                quatf q = glm::rotation(mLastVector, curVec);
                float3x3 rot = glm::toMat3(q);
                mRotation = rot * mRotation;
                mLastVector = curVec;
            }
            break;
        default:
            break;
    }
}

void OrbitController::update() {
    if (!mpCamera) return;

    float3 target = mpCamera->mData.target;
    float3 pos = mpCamera->mData.target +
                 (mRotation * float3(0, 0, 1)) * mCameraDistance;
    mpCamera->mData.posW = target + pos;
    float3 up(0, 1, 0);

    up = mRotation * up;
}

Camera::Camera() { mpController = std::make_unique<OrbitController>(this); }

void Camera::calculateCameraParameters() const {
    mData.viewMat = glm::lookAt(mData.posW, mData.target, mData.up);
    mData.projMat = glm::perspective(mData.fovY, mData.aspectRatio, mData.nearZ,
                                     mData.farZ);

    mData.cameraW = normalize(mData.target - mData.posW) * 100.f;
    mData.cameraU = normalize(cross(mData.cameraW, mData.up));
    mData.cameraV = normalize(cross(mData.cameraU, mData.cameraW));

    const float ulen = 100.f * std::tan(mData.fovY * 0.5f) * mData.aspectRatio;
    mData.cameraU *= ulen;
    const float vlen = 100.f * std::tan(mData.fovY * 0.5f);
    mData.cameraV *= vlen;
}

void Camera::onMouseEvent(const MouseEvent& mouseEvent) const {
    mpController->onMouseEvent(mouseEvent);
    mpController->update();
}

void Camera::bindShaderData(const ShaderVar& var) const {
    calculateCameraParameters();
    var["cameraData"] = mData;
}
} // namespace Voluma