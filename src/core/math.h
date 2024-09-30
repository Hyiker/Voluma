#include <algorithm>

#include "macros.h"
namespace Voluma {
inline float saturate(float num) { return std::max(std::min(num, 1.f), 0.f); }
}  // namespace Voluma