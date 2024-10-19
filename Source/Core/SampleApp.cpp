#include "SampleApp.h"

#include <fmt/format.h>
#include <slang-com-ptr.h>
#include <slang-gfx.h>
#include <slang.h>

#include <cstring>

#include "Core/Camera.h"
#include "Core/Math.h"
#include "Core/Program/Program.h"
#include "Core/Program/ShaderVar.h"
#include "Core/Window.h"
#include "Data/VolData.h"
#include "Error.h"
#include "Utils/Logger.h"
#include "Utils/UiInputs.h"

namespace Voluma {

using namespace gfx;
struct Vertex {
    float position[2];
};

static const int kVertexCount = 3;
static const Vertex kVertexData[kVertexCount] = {
    {{-1.f, 1.f}},
    {{3.f, 1.f}},
    {{-1.f, -3.f}},
};

SampleApp::SampleApp() {
    // Create device
    mpDevice = std::make_shared<Device>();

    mpProgramManager = std::make_shared<ProgramManager>(mpDevice);

    auto gfxDevice = mpDevice->gfxDevice;

    // Create command queue
    ICommandQueue::Desc queueDesc;
    queueDesc.type = ICommandQueue::QueueType::Graphics;
    mQueue = gfxDevice->createCommandQueue(queueDesc);

    // Create framebuffer layout
    IFramebufferLayout::TargetLayout renderTargetLayout = {
        gfx::Format::R8G8B8A8_UNORM, 1};
    IFramebufferLayout::TargetLayout depthLayout = {gfx::Format::D32_FLOAT, 1};
    IFramebufferLayout::Desc framebufferLayoutDesc;
    framebufferLayoutDesc.renderTargetCount = 1;
    framebufferLayoutDesc.renderTargets = &renderTargetLayout;
    framebufferLayoutDesc.depthStencil = &depthLayout;
    gfxDevice->createFramebufferLayout(framebufferLayoutDesc,
                                       mFramebufferLayout.writeRef());

    // Create window
    Window::Desc windowDesc;
    mpWindow = Window::create(windowDesc, this);
    // Create swapchain
    gfx::ISwapchain::Desc swapchainDesc = {};
    swapchainDesc.format = gfx::Format::R8G8B8A8_UNORM;
    swapchainDesc.width = windowDesc.width;
    swapchainDesc.height = windowDesc.height;
    swapchainDesc.imageCount = kSwapChainImageCount;
    swapchainDesc.queue = mQueue;
    mSwapchain = gfxDevice->createSwapchain(
        swapchainDesc,
#if VL_WINDOWS
        gfx::WindowHandle::FromHwnd(mpWindow->getNativeHandle())
#elif VL_MACOSX
        gfx::WindowHandle::FromNSwnd(mpWindow->getNativeHandle())
#endif
    );
    VL_ASSERT(mSwapchain != nullptr);
    createFramebuffers();

    // Create resource heap
    for (int i = 0; i < kSwapChainImageCount; i++) {
        ITransientResourceHeap::Desc transientHeapDesc = {};
        transientHeapDesc.constantBufferSize = 16 * 1024 * 1024;
        transientHeapDesc.flags =
            gfx::ITransientResourceHeap::Flags::AllowResizing;

        gfx::ComPtr<ITransientResourceHeap> transientHeap;

        if (SLANG_FAILED(gfxDevice->createTransientResourceHeap(
                transientHeapDesc, transientHeap.writeRef()))) {
            logError("Failed to create transient resource heap");
        }
        mTransientHeaps.push_back(transientHeap);
    }

    // Create render pass

    gfx::IRenderPassLayout::Desc renderPassDesc = {};
    renderPassDesc.framebufferLayout = mFramebufferLayout;
    renderPassDesc.renderTargetCount = 1;
    IRenderPassLayout::TargetAccessDesc renderTargetAccess = {};
    IRenderPassLayout::TargetAccessDesc depthStencilAccess = {};
    renderTargetAccess.loadOp = IRenderPassLayout::TargetLoadOp::Clear;
    renderTargetAccess.storeOp = IRenderPassLayout::TargetStoreOp::Store;
    renderTargetAccess.initialState = ResourceState::Undefined;
    renderTargetAccess.finalState = ResourceState::Present;
    depthStencilAccess.loadOp = IRenderPassLayout::TargetLoadOp::Clear;
    depthStencilAccess.storeOp = IRenderPassLayout::TargetStoreOp::Store;
    depthStencilAccess.initialState = ResourceState::DepthWrite;
    depthStencilAccess.finalState = ResourceState::DepthWrite;
    renderPassDesc.renderTargetAccess = &renderTargetAccess;
    renderPassDesc.depthStencilAccess = &depthStencilAccess;
    mRenderPass = gfxDevice->createRenderPassLayout(renderPassDesc);

    // First, we create an input layout:
    //
    InputElementDesc inputElements[] = {
        {"POSITION", 0, Format::R32G32_FLOAT, offsetof(Vertex, position), 0},
    };
    auto inputLayout =
        gfxDevice->createInputLayout(sizeof(Vertex), &inputElements[0], 1);
    VL_ASSERT(inputLayout != nullptr);

    // Next we allocate a vertex buffer for our pre-initialized
    // vertex data.
    //
    IBufferResource::Desc vertexBufferDesc;
    vertexBufferDesc.type = IResource::Type::Buffer;
    vertexBufferDesc.sizeInBytes = kVertexCount * sizeof(Vertex);
    vertexBufferDesc.defaultState = ResourceState::VertexBuffer;
    mVertexBuffer =
        gfxDevice->createBufferResource(vertexBufferDesc, &kVertexData[0]);
    VL_ASSERT(mVertexBuffer != nullptr);

    // Create present pipeline
    {
        Slang::ComPtr<gfx::IShaderProgram> graphicsProgram =
            mpProgramManager->createProgram(
                "Shaders/Present.raster.slang",
                {{"vertexMain", ShaderType::Vertex},
                 {"fragmentMain", ShaderType::Pixel}});

        GraphicsPipelineStateDesc desc;
        desc.inputLayout = inputLayout;
        desc.program = graphicsProgram;
        desc.framebufferLayout = mFramebufferLayout;
        mPresentPipelineState = gfxDevice->createGraphicsPipelineState(desc);
        VL_ASSERT(mPresentPipelineState != nullptr);
    }

    // Create compute pipeline
    {
        Slang::ComPtr<gfx::IShaderProgram> computeProgram =
            mpProgramManager->createProgram("Shaders/RayMarching.cs.slang",
                                            {{"main", ShaderType::Compute}});

        ComputePipelineStateDesc desc;
        desc.program = computeProgram;
        mComputePipelineState = gfxDevice->createComputePipelineState(desc);
        VL_ASSERT(mComputePipelineState != nullptr);

        createComputeTexture();
    }
}

SampleApp::SampleApp(SampleApp&& other) noexcept
    : mpWindow(other.mpWindow), mpDevice(other.mpDevice) {
    other.mpDevice = nullptr;
}

void SampleApp::createFramebuffers() {
    mFramebuffers.clear();
    int width = mSwapchain->getDesc().width;
    int height = mSwapchain->getDesc().height;

    auto colorFormat = mSwapchain->getDesc().format;

    auto gfxDevice = mpDevice->gfxDevice;
    for (uint32_t i = 0; i < kSwapChainImageCount; i++) {
        // Depth texture
        gfx::ITextureResource::Desc depthBufferDesc;
        depthBufferDesc.type = IResource::Type::Texture2D;
        depthBufferDesc.size.width = width;
        depthBufferDesc.size.height = height;
        depthBufferDesc.size.depth = 1;
        depthBufferDesc.format = gfx::Format::D32_FLOAT;
        depthBufferDesc.defaultState = ResourceState::DepthWrite;
        depthBufferDesc.allowedStates =
            ResourceStateSet(ResourceState::DepthWrite);
        ClearValue depthClearValue = {};
        depthBufferDesc.optimalClearValue = &depthClearValue;
        ComPtr<gfx::ITextureResource> depthBufferResource =
            gfxDevice->createTextureResource(depthBufferDesc, nullptr);

        gfx::IResourceView::Desc depthBufferViewDesc;
        memset(&depthBufferViewDesc, 0, sizeof(depthBufferViewDesc));
        depthBufferViewDesc.format = gfx::Format::D32_FLOAT;
        depthBufferViewDesc.renderTarget.shape =
            gfx::IResource::Type::Texture2D;
        depthBufferViewDesc.type = gfx::IResourceView::Type::DepthStencil;
        ComPtr<gfx::IResourceView> dsv = gfxDevice->createTextureView(
            depthBufferResource.get(), depthBufferViewDesc);

        // Color texture
        ComPtr<gfx::ITextureResource> colorBuffer;
        mSwapchain->getImage(i, colorBuffer.writeRef());

        gfx::IResourceView::Desc colorBufferViewDesc;
        memset(&colorBufferViewDesc, 0, sizeof(colorBufferViewDesc));
        colorBufferViewDesc.format = colorFormat;
        colorBufferViewDesc.renderTarget.shape =
            gfx::IResource::Type::Texture2D;
        colorBufferViewDesc.type = gfx::IResourceView::Type::RenderTarget;
        ComPtr<gfx::IResourceView> rtv = gfxDevice->createTextureView(
            colorBuffer.get(), colorBufferViewDesc);

        gfx::IFramebuffer::Desc framebufferDesc;
        framebufferDesc.renderTargetCount = 1;
        framebufferDesc.depthStencilView = dsv.get();
        framebufferDesc.renderTargetViews = rtv.readRef();
        framebufferDesc.layout = mFramebufferLayout;
        ComPtr<gfx::IFramebuffer> frameBuffer =
            gfxDevice->createFramebuffer(framebufferDesc);

        mFramebuffers.push_back(frameBuffer);
    }
}

void SampleApp::createComputeTexture() {
    auto gfxDevice = mpDevice->gfxDevice;

    int width = mSwapchain->getDesc().width;
    int height = mSwapchain->getDesc().height;
    ITextureResource::Desc resultTextureDesc = {};
    resultTextureDesc.type = IResource::Type::Texture2D;
    resultTextureDesc.numMipLevels = 1;
    resultTextureDesc.size.width = width;
    resultTextureDesc.size.height = height;
    resultTextureDesc.size.depth = 1;
    resultTextureDesc.defaultState = ResourceState::UnorderedAccess;
    resultTextureDesc.format = Format::R32G32B32A32_FLOAT;
    auto resource = gfxDevice->createTextureResource(resultTextureDesc);
    IResourceView::Desc resultUAVDesc = {};
    resultUAVDesc.format = resultTextureDesc.format;
    resultUAVDesc.type = IResourceView::Type::UnorderedAccess;
    auto textureUAV = gfxDevice->createTextureView(resource, resultUAVDesc);

    mComputeTexture.resource = resource;
    mComputeTexture.view = textureUAV;
}

void SampleApp::createComputeBuffer() {
    gfx::IBufferResource::Desc bufferDesc = {};
    bufferDesc.allowedStates.add(ResourceState::ShaderResource);
    bufferDesc.allowedStates.add(ResourceState::UnorderedAccess);
    bufferDesc.allowedStates.add(ResourceState::CopyDestination);
    bufferDesc.allowedStates.add(ResourceState::General);
    bufferDesc.sizeInBytes = mpVolData->getSliceCount() *
                             mpVolData->getColWidth() *
                             mpVolData->getRowWidth() * sizeof(float);
    bufferDesc.type = IResource::Type::Buffer;
    mVolBuffer.resource = mpDevice->gfxDevice->createBufferResource(bufferDesc);

    IResourceView::Desc desc = {};
    desc.type = IResourceView::Type::UnorderedAccess;
    mVolBuffer.view = mpDevice->gfxDevice->createBufferView(mVolBuffer.resource,
                                                            nullptr, desc);
}

void SampleApp::handleRenderFrame() {
    int framebufferIndex = mSwapchain->acquireNextImage();

    mTransientHeaps[framebufferIndex]->synchronizeAndReset();
    executeRenderFrame(framebufferIndex);
    mTransientHeaps[framebufferIndex]->finish();
}
void SampleApp::handleMouseEvent(const MouseEvent& mouseEvent) {
    mCamera.onMouseEvent(mouseEvent);
}

void SampleApp::handleKeyboardEvent(const KeyboardEvent& keyEvent) {
    if (keyEvent.type == KeyboardEvent::Type::KeyReleased) {
        switch (keyEvent.key) {
            case Input::Key::Escape: {
                mpWindow->shutdown();
                break;
            }
            default:
                break;
        }
    }
}

void SampleApp::loadFromDisk(const std::string& filename) {
    mpVolData = VolData::loadFromDisk(filename);

    logInfo("Slice count: {}", mpVolData->getSliceCount());
    logInfo("patient info: {}", mpVolData->getPatientData());
    logInfo("scan meta: {}", mpVolData->getScanMetaData());

    createComputeBuffer();
}

void SampleApp::beginLoop() { mpWindow->msgLoop(); }

void SampleApp::executeRenderFrame(int framebufferIndex) {
    auto gfxDevice = mpDevice->gfxDevice;

    int width = mSwapchain->getDesc().width;
    int height = mSwapchain->getDesc().height;
    static bool isBufferCopied = false;
    if (!isBufferCopied) {
        ComPtr<ICommandBuffer> resourceCommandBuffer =
            mTransientHeaps[framebufferIndex]->createCommandBuffer();
        auto resourceEncoder = resourceCommandBuffer->encodeResourceCommands();

        resourceEncoder->bufferBarrier(mVolBuffer.resource,
                                       ResourceState::Undefined,
                                       ResourceState::CopyDestination);
        resourceEncoder->uploadBufferData(
            mVolBuffer.resource, 0, mpVolData->getBufferData().size(),
            (void*)mpVolData->getBufferData().data());
        resourceEncoder->endEncoding();
        resourceCommandBuffer->close();
        mQueue->executeCommandBuffer(resourceCommandBuffer);
        isBufferCopied = true;
    }

    {
        ComPtr<ICommandBuffer> computeCommandBuffer =
            mTransientHeaps[framebufferIndex]->createCommandBuffer();
        auto renderEncoder = computeCommandBuffer->encodeComputeCommands();

        auto rootObject = renderEncoder->bindPipeline(mComputePipelineState);

        renderEncoder->bufferBarrier(mVolBuffer.resource,
                                     ResourceState::CopyDestination,
                                     ResourceState::UnorderedAccess);

        auto deviceInfo = gfxDevice->getDeviceInfo();

        ShaderVar rootVar(rootObject);
        mCamera.bindShaderData(rootVar);
        rootVar["frameDim"] = uint2(width, height);
        rootVar["dstTex"] = mComputeTexture;
        rootVar["volBuffer"] = mVolBuffer;

        if (SLANG_FAILED(
                renderEncoder->dispatchCompute(width / 16, height / 16, 1))) {
            logError("dispatchCompute failed");
        }
        renderEncoder->endEncoding();
        computeCommandBuffer->close();
        mQueue->executeCommandBuffer(computeCommandBuffer);
    }

    {
        ComPtr<ICommandBuffer> presentCommandBuffer =
            mTransientHeaps[framebufferIndex]->createCommandBuffer();
        auto renderEncoder = presentCommandBuffer->encodeRenderCommands(
            mRenderPass, mFramebuffers[framebufferIndex]);

        gfx::Viewport viewport = {};
        viewport.maxZ = 1.0f;
        viewport.extentX = width;
        viewport.extentY = height;
        renderEncoder->setViewportAndScissor(viewport);

        auto rootObject = renderEncoder->bindPipeline(mPresentPipelineState);

        auto deviceInfo = gfxDevice->getDeviceInfo();

        ShaderVar rootVar(rootObject);
        rootVar["srcTex"] = mComputeTexture;

        // We also need to set up a few pieces of fixed-function pipeline
        // state that are not bound by the pipeline state above.
        //
        renderEncoder->setVertexBuffer(0, mVertexBuffer);
        renderEncoder->setPrimitiveTopology(PrimitiveTopology::TriangleList);

        // Finally, we are ready to issue a draw call for a single triangle.
        //
        renderEncoder->draw(kVertexCount);
        renderEncoder->endEncoding();
        presentCommandBuffer->close();
        mQueue->executeCommandBuffer(presentCommandBuffer);
    }

    mSwapchain->present();
}
SampleApp::~SampleApp() = default;

} // namespace Voluma