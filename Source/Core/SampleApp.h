#pragma once
#include <slang-com-helper.h>
#include <slang-gfx.h>

#include <cstdint>
#include <vector>

#include "Texture.h"
#include "Window.h"
#include "slang-com-ptr.h"


namespace Voluma {
class SampleApp : public Window::ICallbacks, public gfx::IDebugCallback {
   public:
    SampleApp();

    SampleApp(const SampleApp &other) = delete;
    SampleApp(SampleApp &&other) noexcept;

    void createFramebuffers();

    void createComputeTexture();

    Slang::ComPtr<gfx::IShaderProgram> createGraphicsShader();

    Slang::ComPtr<gfx::IShaderProgram> createComputeShader();

    void beginLoop();

    virtual void handleWindowSizeChange() override {}
    virtual void handleRenderFrame() override;
    virtual void handleKeyboardEvent(const KeyboardEvent &keyEvent) override;
    virtual void handleMouseEvent(const MouseEvent &mouseEvent) override {}
    virtual void handleDroppedFile(const std::filesystem::path &path) override {
    }

    virtual SLANG_NO_THROW void SLANG_MCALL
    handleMessage(gfx::DebugMessageType type, gfx::DebugMessageSource source,
                  const char *message) override;

    SampleApp &operator=(const SampleApp &other) = delete;
    SampleApp &operator=(SampleApp &&other) noexcept;

    ~SampleApp();

   private:
    void executeRenderFrame(int framebufferIndex);

    static const int kSwapChainImageCount = 2;

    std::shared_ptr<Window> mpWindow;

    Slang::ComPtr<gfx::IDevice> mDevice; ///< GPU device.
    Slang::ComPtr<gfx::ISwapchain> mSwapchain;
    Slang::ComPtr<gfx::ICommandQueue> mQueue; ///< Command queue.
    Slang::ComPtr<gfx::IFramebufferLayout> mFramebufferLayout;
    std::vector<Slang::ComPtr<gfx::IFramebuffer>> mFramebuffers;
    std::vector<Slang::ComPtr<gfx::ITransientResourceHeap>> mTransientHeaps;
    Slang::ComPtr<gfx::IRenderPassLayout> mRenderPass;

    Slang::ComPtr<gfx::IBufferResource> mVertexBuffer;        ///<
    Slang::ComPtr<gfx::IPipelineState> mPresentPipelineState; ///<

    Texture mComputeTexture;
    Slang::ComPtr<gfx::IPipelineState> mComputePipelineState; ///<
};
} // namespace Voluma