#pragma once
#include <slang-com-ptr.h>
#include <slang-gfx.h>
#include <slang.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Core/Device.h"
#include "Core/Macros.h"

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

using ProgramEntryPoint = std::pair<std::string, ShaderType>;

class VL_API ProgramManager {
   public:
    ProgramManager(std::shared_ptr<Device> device);
    Slang::ComPtr<gfx::IShaderProgram> createProgram(
        std::string_view filePath,
        std::vector<ProgramEntryPoint> entryPoints) const;

   private:
    std::shared_ptr<Device> mpDevice;
};

} // namespace Voluma
