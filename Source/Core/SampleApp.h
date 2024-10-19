#pragma once
#include <slang-com-helper.h>
#include <slang-com-ptr.h>
#include <slang-gfx.h>

#include <cstdint>
#include <vector>

#include "Buffer.h"
#include "Core/Camera.h"
#include "Core/Program/Program.h"
#include "Data/VolData.h"
#include "Device.h"
#include "Texture.h"
#include "Window.h"

namespace Voluma {
class SampleApp : public Window::ICallbacks {
   public:
    SampleApp();

    SampleApp(const SampleApp &other) = delete;
    SampleApp(SampleApp &&other) noexcept;

    void createFramebuffers();

    void createComputeTexture();

    void createComputeBuffer();

    Slang::ComPtr<gfx::IShaderProgram> createGraphicsShader();

    Slang::ComPtr<gfx::IShaderProgram> createComputeShader();

    void loadFromDisk(const std::string &filename);

    void beginLoop();

    virtual void handleWindowSizeChange() override {}
    virtual void handleRenderFrame() override;
    virtual void handleKeyboardEvent(const KeyboardEvent &keyEvent) override;
    virtual void handleMouseEvent(const MouseEvent &mouseEvent) override;
    virtual void handleDroppedFile(const std::filesystem::path &path) override {
    }

    SampleApp &operator=(const SampleApp &other) = delete;
    SampleApp &operator=(SampleApp &&other) noexcept;

    ~SampleApp();

   private:
    void executeRenderFrame(int framebufferIndex);

    static const int kSwapChainImageCount = 2;

    Camera mCamera;

    std::shared_ptr<Window> mpWindow;

    std::shared_ptr<Device> mpDevice; ///< GPU device.
    Slang::ComPtr<gfx::ISwapchain> mSwapchain;
    Slang::ComPtr<gfx::ICommandQueue> mQueue; ///< Command queue.
    Slang::ComPtr<gfx::IFramebufferLayout> mFramebufferLayout;
    std::vector<Slang::ComPtr<gfx::IFramebuffer>> mFramebuffers;
    std::vector<Slang::ComPtr<gfx::ITransientResourceHeap>> mTransientHeaps;
    Slang::ComPtr<gfx::IRenderPassLayout> mRenderPass;

    std::shared_ptr<ProgramManager> mpProgramManager;

    Slang::ComPtr<gfx::IBufferResource> mVertexBuffer;        ///<
    Slang::ComPtr<gfx::IPipelineState> mPresentPipelineState; ///<

    Texture mComputeTexture;
    Buffer mVolBuffer;
    Slang::ComPtr<gfx::IPipelineState> mComputePipelineState; ///<

    std::shared_ptr<VolData> mpVolData;
};
} // namespace Voluma