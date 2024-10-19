#pragma once

#include <slang-gfx.h>

namespace Voluma {

struct Device {
    gfx::ComPtr<gfx::IDevice> gfxDevice;
    gfx::ComPtr<slang::IGlobalSession> slangGlobalSession;

    Device();
};

} // namespace Voluma