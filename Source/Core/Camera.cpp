#include "Camera.h"

#include "Program/ShaderVar.h"
#include "glm/gtc/matrix_transform.hpp"
namespace Voluma {

static_assert(sizeof(CameraData) % (sizeof(float4)) == 0,
              "CameraData size should be a multiple of 16");

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
    switch (mouseEvent.type) {
        case MouseEvent::Type::Wheel:
            mData.posW -= glm::normalize(mData.posW - mData.target) *
                          (mouseEvent.wheelDelta.y * 0.2f);
            logInfo("Camera.posW = ({}, {}, {})", mData.posW.x, mData.posW.y,
                    mData.posW.z);
            break;
    }
}

void Camera::bindShaderData(const ShaderVar& var) const {
    calculateCameraParameters();
    var["cameraData"] = mData;
}
} // namespace Voluma