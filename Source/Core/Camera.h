#pragma once
#include <glm/ext/matrix_transform.hpp>

#include "CameraData.slang"
#include "Core/Math.h"
#include "Utils/UiInputs.h"
namespace Voluma {
class ShaderVar;
class Camera;

class OrbitController {
   public:
    OrbitController(Camera* pCamera) ;

    void onMouseEvent(const MouseEvent& mouseEvent);
    void update();

   private:
    Camera* mpCamera;

    float mCameraDistance;

    float3x3 mRotation = glm::identity<float3x3>();
    float3 mLastVector;
    bool mIsLeftButtonDown = false;
};

class VL_API Camera {
   public:
    Camera();

    void bindShaderData(const ShaderVar& var) const;
    void onMouseEvent(const MouseEvent& mouseEvent) const;

   private:
    friend class OrbitController;

    void calculateCameraParameters() const;
    mutable CameraData mData;

    std::unique_ptr<OrbitController> mpController;
};
} // namespace Voluma