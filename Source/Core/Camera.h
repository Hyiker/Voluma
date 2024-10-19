#pragma once
#include "CameraData.slang"
#include "Utils/UiInputs.h"
namespace Voluma {
class ShaderVar;
class VL_API Camera {
   public:
    void bindShaderData(const ShaderVar& var) const;
    void onMouseEvent(const MouseEvent& mouseEvent) const;

   private:
    void calculateCameraParameters() const;
    mutable CameraData mData;
};
} // namespace Voluma