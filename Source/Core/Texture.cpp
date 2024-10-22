#include "Texture.h"

#include "Core/Device.h"
#include "Utils/Logger.h"

namespace Voluma {

Texture::Texture(const Device::SharedPtr& pDevice,
                 gfx::ITextureResource::Desc textureDesc,
                 gfx::IResourceView::Desc viewDesc,
                 const gfx::ITextureResource::SubresourceData* pInitData)
    : mTextureDesc(textureDesc), mViewDesc(viewDesc) {
    auto resource =
        pDevice->getGfxDevice()->createTextureResource(textureDesc, pInitData);

    auto view = pDevice->getGfxDevice()->createTextureView(resource, viewDesc);

    mResource = resource;
    mView = view;
}

gfx::Format Texture::getFormat() const { return mTextureDesc.format; }

} // namespace Voluma