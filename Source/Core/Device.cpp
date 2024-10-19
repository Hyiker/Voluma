#include "Device.h"

#include <slang-gfx.h>

#include "Utils/Logger.h"

namespace Voluma {
using namespace gfx;

class GFXDebugCallBack : public gfx::IDebugCallback {
    virtual SLANG_NO_THROW void SLANG_MCALL
    handleMessage(gfx::DebugMessageType type, gfx::DebugMessageSource source,
                  const char* message) override {
        if (type == gfx::DebugMessageType::Error) {
            logError("GFX Error: {}", message);
        } else if (type == gfx::DebugMessageType::Warning) {
            logWarning("GFX Warning: {}", message);
        } else {
            logDebug("GFX Info: {}", message);
        }
    }
};

GFXDebugCallBack gGFXDebugCallBack;

Device::Device() {
    slang::createGlobalSession(slangGlobalSession.writeRef());

    gfx::IDevice::Desc deviceDesc = {};
    // deviceDesc.deviceType = gfx::DeviceType::DirectX12;
    deviceDesc.slang.slangGlobalSession = slangGlobalSession;

    if (SLANG_FAILED(gfxSetDebugCallback(&gGFXDebugCallBack))) {
        logFatal("gfxSetDebugCallback failed");
    }
    gfxEnableDebugLayer();

    if (SLANG_FAILED(gfxCreateDevice(&deviceDesc, gfxDevice.writeRef()))) {
        logFatal("Failed to create GPU device");
    }
}
} // namespace Voluma