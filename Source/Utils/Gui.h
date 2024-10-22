#pragma once
#include <slang-com-ptr.h>
#include <slang-gfx.h>
#include <slang.h>

#include <memory>

#include "Core/Program/Program.h"
#include "Utils/UiInputs.h"

struct ImGuiContext;

namespace Voluma {
class Window;
class Device;
class Gui {
   public:
    Gui(Window* window, const std::shared_ptr<Device>& device,
        const std::shared_ptr<ProgramManager>& programManager,
        Slang::ComPtr<gfx::ICommandQueue> queue,
        Slang::ComPtr<gfx::IFramebufferLayout> framebufferLayout);
    ~Gui();

    void beginFrame();

    void endFrame(gfx::ITransientResourceHeap* transientHeap,
                  gfx::IFramebuffer* framebuffer);

    bool onMouseEvent(const MouseEvent& mouseEvent);

   private:
    static constexpr uint32_t kBufferCacheCount = 3;

    struct BufferCache {
        Slang::ComPtr<gfx::IBufferResource> vertexBuffer;
        Slang::ComPtr<gfx::IBufferResource> indexBuffer;
        uint32_t vertexBufferSize = 0;
        uint32_t indexBufferSize = 0;
    };

    BufferCache getBufferCache(uint32_t vertexCount, uint32_t indexCount);

    BufferCache mRotateBufferCache[kBufferCacheCount];
    uint32_t mCacheIndex = 0;

    ImGuiContext* mpContext;

    Slang::ComPtr<gfx::ISamplerState> mpSampler;

    std::shared_ptr<Texture> mpFontTex;
    std::shared_ptr<Device> mpDevice;
    Slang::ComPtr<gfx::ICommandQueue> mpQueue;
    Slang::ComPtr<gfx::IRenderPassLayout> mpRenderPass;
    Slang::ComPtr<gfx::IPipelineState> mpPipelineState;
    Slang::ComPtr<gfx::ISamplerState> mpSamplerState;
};

} // namespace Voluma