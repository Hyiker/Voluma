#pragma once
#include "Macros.h"
#include <algorithm>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <glm/mat2x2.hpp>
#include <glm/mat2x3.hpp>
#include <glm/mat2x4.hpp>
#include <glm/mat3x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat3x4.hpp>
#include <glm/mat4x2.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vector_relational.hpp>

namespace Voluma {

using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;

using float2x2 = glm::mat2x2;
using float3x3 = glm::mat3x3;
using float4x4 = glm::mat4x4;

inline float saturate(float num) { return std::max(std::min(num, 1.f), 0.f); }
} // namespace Voluma