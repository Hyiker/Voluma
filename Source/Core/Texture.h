#pragma once
#include <slang-gfx.h>
#include <slang.h>

#include <memory>

#include "Macros.h"
namespace Voluma {
class Device;
class VL_API Texture {
   public:
    using SharedPtr = std::shared_ptr<Texture>;

    gfx::ComPtr<gfx::IResourceView> getView() const { return mView; }

    gfx::ComPtr<gfx::ITextureResource> getResource() const { return mResource; }

    gfx::Format getFormat() const;

    Texture(const std::shared_ptr<Device>& pDevice,
            gfx::ITextureResource::Desc textureDesc,
            gfx::IResourceView::Desc viewDesc,
            const gfx::ITextureResource::SubresourceData* pInitData);

   private:
    gfx::ITextureResource::Desc mTextureDesc;
    gfx::IResourceView::Desc mViewDesc;

    gfx::ComPtr<gfx::ITextureResource> mResource;
    gfx::ComPtr<gfx::IResourceView> mView;
};
} // namespace Voluma