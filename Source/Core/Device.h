#pragma once

#include <slang-gfx.h>

#include <memory>

namespace Voluma {

class Texture;

class Device : public std::enable_shared_from_this<Device> {
   public:
    using SharedPtr = std::shared_ptr<Device>;

    Device();

    std::shared_ptr<Texture> createTexture(
        gfx::ITextureResource::Desc textureDesc,
        gfx::IResourceView::Desc viewDesc,
        const gfx::ITextureResource::SubresourceData* pInitData = nullptr);

    gfx::ComPtr<gfx::IDevice> getGfxDevice() const { return mGfxDevice; }

    gfx::ComPtr<slang::IGlobalSession> getGlobalSession() const {
        return mSlangGlobalSession;
    }

   private:
    gfx::ComPtr<gfx::IDevice> mGfxDevice;
    gfx::ComPtr<slang::IGlobalSession> mSlangGlobalSession;
};

} // namespace Voluma