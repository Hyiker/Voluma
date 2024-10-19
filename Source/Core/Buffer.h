#pragma once
#include <slang-gfx.h>
#include <slang.h>
namespace Voluma {
struct Buffer {
    gfx::ComPtr<gfx::IBufferResource> resource;
    gfx::ComPtr<gfx::IResourceView> view;
};
} // namespace Voluma