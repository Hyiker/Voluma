#pragma once
#include <slang-gfx.h>
#include <slang.h>

#include "core/Macros.h"
namespace Voluma {
enum class ShaderType {
    Vertex,        ///< Vertex shader
    Pixel,         ///< Pixel shader
    Geometry,      ///< Geometry shader
    Hull,          ///< Hull shader (AKA Tessellation control shader)
    Domain,        ///< Domain shader (AKA Tessellation evaluation shader)
    Compute,       ///< Compute shader
    RayGeneration, ///< Ray generation shader
    Intersection,  ///< Intersection shader
    AnyHit,        ///< Any hit shader
    ClosestHit,    ///< Closest hit shader
    Miss,          ///< Miss shader
    Callable,      ///< Callable shader
    Count          ///< Shader Type count
};

class VL_API ShaderProgram {
   public:

   private:
    // const Slang::ProgramDesc mDesc;

    Slang::ComPtr<gfx::IShaderProgram> mGfxProgram;
};
} // namespace Voluma
