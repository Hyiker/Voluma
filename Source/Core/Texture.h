#pragma once
#include <slang-gfx.h>
#include <slang.h>
namespace Voluma {
struct Texture {
    gfx::ComPtr<gfx::ITextureResource> resource;
    gfx::ComPtr<gfx::IResourceView> view;
};
} // namespace Voluma