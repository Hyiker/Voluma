#pragma once
#include "Utils/HostDevice.slangh"

BEGIN_NAMESPACE_VL

struct CameraData {
    float4x4 viewMat;     ///< Camera view matrix.
    float4x4 projMat;     ///< Camera projection matrix.
    float4x4 viewProjMat; ///< Camera view-projection matrix.

    float3 posW = float3(0, 0, 3);    ///< Camera world-space position.
    float fovY = 21.0f;               ///< Camera Y axis fov in degree.
    float3 up = float3(0, 1, 0);      ///< Camera world-space up vector.
    float aspectRatio = 1.7777777f;   ///< Camera film frame aspect ratio, equal to frameWidth / frameHeight
    float3 target = float3(0, 0, 0); ///< Camera target point in world-space.
    float nearZ = 0.1f;               ///< Camera near plane.
    float3 cameraU = float3(0, 0, 1); ///< Camera base vector U. Normalized it indicates the right image plane vector. The length is
                                      ///< dependent on the FOV.
    float farZ = 1000.0f;             ///< Camera far plane.
    float3 cameraV = float3(0, 1, 0); ///< Camera base vector V. Normalized it indicates the up image plane vector. The length is dependent
                                      ///< on the FOV.
    float _padding0;
    float3 cameraW = float3(1, 0, 0); ///< Camera base vector W. Normalized it indicates the forward direction. The length is the camera
                                      ///< focal distance.
    float _padding1;
};
END_NAMESPACE_VL