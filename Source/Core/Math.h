#pragma once
#include <algorithm>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat2x3.hpp>
#include <glm/mat2x4.hpp>
#include <glm/mat3x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat3x4.hpp>
#include <glm/mat4x2.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/vector_relational.hpp>
#include <numbers>

#include "Macros.h"
#include "Utils/Logger.h"
#include "glm/ext/vector_uint2.hpp"

namespace Voluma {

using uint2 = glm::uvec2;
using uint3 = glm::uvec3;
using uint4 = glm::uvec4;

using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;

using float2x2 = glm::mat2x2;
using float3x3 = glm::mat3x3;
using float4x4 = glm::mat4x4;

using quatf = glm::f32quat;

inline float saturate(float num) { return std::max(std::min(num, 1.f), 0.f); }

inline float3 project2DCrdToUnitSphere(float2 xy) {
    float xyLengthSquared = dot(xy, xy);

    float z = 0;
    if (xyLengthSquared < 1) {
        z = std::sqrt(1 - xyLengthSquared);
    } else {
        xy = normalize(xy);
    }
    return float3(xy.x, xy.y, z);
}

} // namespace Voluma