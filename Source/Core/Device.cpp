#include "Device.h"

#include <slang-gfx.h>

#include "Texture.h"
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

gfx::DeviceType getPlatformDevice() {
#if VL_WINDOWS
    return gfx::DeviceType::DirectX12;
#elif VL_MACOSX
    return gfx::DeviceType::Metal;
#else
    return gfx::DeviceType::Vulkan;
#endif
}

Device::Device() {

    gfxEnableDebugLayer();
    if (SLANG_FAILED(gfxSetDebugCallback(&gGFXDebugCallBack))) {
        logFatal("gfxSetDebugCallback failed");
    }

    slang::createGlobalSession(mSlangGlobalSession.writeRef());
    
    gfx::IDevice::Desc deviceDesc = {};
    deviceDesc.deviceType = getPlatformDevice();
    deviceDesc.slang.slangGlobalSession = mSlangGlobalSession;

    if (SLANG_FAILED(gfxCreateDevice(&deviceDesc, mGfxDevice.writeRef()))) {
        logFatal("Failed to create GPU device");
    }
}

std::shared_ptr<Texture> Device::createTexture(
    gfx::ITextureResource::Desc textureDesc, gfx::IResourceView::Desc viewDesc,
    const gfx::ITextureResource::SubresourceData* pInitData) {
    return std::make_shared<Texture>(this->shared_from_this(), textureDesc,
                                     viewDesc, pInitData);
}
} // namespace Voluma