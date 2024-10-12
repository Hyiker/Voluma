#include "SampleApp.h"

#include <fmt/format.h>
#include <slang-com-ptr.h>
#include <slang-gfx.h>
#include <slang.h>

#include <cstring>

#include "Core/Math.h"
#include "Core/Program/ShaderVar.h"
#include "Core/Window.h"
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
    gfxEnableDebugLayer();
    gfxSetDebugCallback(this);
    // Create device
    IDevice::Desc deviceDesc = {};
    gfxCreateDevice(&deviceDesc, mDevice.writeRef());

    // Create command queue
    ICommandQueue::Desc queueDesc = {ICommandQueue::QueueType::Graphics};
    mDevice->createCommandQueue(queueDesc, mQueue.writeRef());

    // Create framebuffer layout
    IFramebufferLayout::TargetLayout renderTargetLayout = {
        gfx::Format::R8G8B8A8_UNORM, 1};
    IFramebufferLayout::TargetLayout depthLayout = {gfx::Format::D32_FLOAT, 1};
    IFramebufferLayout::Desc framebufferLayoutDesc;
    framebufferLayoutDesc.renderTargetCount = 1;
    framebufferLayoutDesc.renderTargets = &renderTargetLayout;
    framebufferLayoutDesc.depthStencil = &depthLayout;
    mDevice->createFramebufferLayout(framebufferLayoutDesc,
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
    mSwapchain = mDevice->createSwapchain(
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
        transientHeapDesc.constantBufferSize = 4096 * 1024;

        auto transientHeap =
            mDevice->createTransientResourceHeap(transientHeapDesc);
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
    mRenderPass = mDevice->createRenderPassLayout(renderPassDesc);

    // First, we create an input layout:
    //
    InputElementDesc inputElements[] = {
        {"POSITION", 0, Format::R32G32_FLOAT, offsetof(Vertex, position)},
    };
    auto inputLayout =
        mDevice->createInputLayout(sizeof(Vertex), &inputElements[0], 1);
    VL_ASSERT(inputLayout != nullptr);

    // Next we allocate a vertex buffer for our pre-initialized
    // vertex data.
    //
    IBufferResource::Desc vertexBufferDesc;
    vertexBufferDesc.type = IResource::Type::Buffer;
    vertexBufferDesc.sizeInBytes = kVertexCount * sizeof(Vertex);
    vertexBufferDesc.defaultState = ResourceState::VertexBuffer;
    mVertexBuffer =
        mDevice->createBufferResource(vertexBufferDesc, &kVertexData[0]);
    VL_ASSERT(mVertexBuffer != nullptr);

    // Create present pipeline
    {
        Slang::ComPtr<gfx::IShaderProgram> graphicsProgram =
            createGraphicsShader();

        GraphicsPipelineStateDesc desc;
        desc.inputLayout = inputLayout;
        desc.program = graphicsProgram;
        desc.framebufferLayout = mFramebufferLayout;
        mPresentPipelineState = mDevice->createGraphicsPipelineState(desc);
        VL_ASSERT(mPresentPipelineState != nullptr);
    }

    // Create compute pipeline
    {
        Slang::ComPtr<gfx::IShaderProgram> computeProgram =
            createComputeShader();

        ComputePipelineStateDesc desc;
        desc.program = computeProgram;
        mComputePipelineState = mDevice->createComputePipelineState(desc);
        VL_ASSERT(mComputePipelineState != nullptr);

        createComputeTexture();
    }
}

SampleApp::SampleApp(SampleApp&& other) noexcept
    : mpWindow(other.mpWindow), mDevice(other.mDevice) {
    other.mDevice = nullptr;
}

void SampleApp::createFramebuffers() {
    mFramebuffers.clear();
    int width = mSwapchain->getDesc().width;
    int height = mSwapchain->getDesc().height;

    auto colorFormat = mSwapchain->getDesc().format;
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
            mDevice->createTextureResource(depthBufferDesc, nullptr);

        gfx::IResourceView::Desc depthBufferViewDesc;
        memset(&depthBufferViewDesc, 0, sizeof(depthBufferViewDesc));
        depthBufferViewDesc.format = gfx::Format::D32_FLOAT;
        depthBufferViewDesc.renderTarget.shape =
            gfx::IResource::Type::Texture2D;
        depthBufferViewDesc.type = gfx::IResourceView::Type::DepthStencil;
        ComPtr<gfx::IResourceView> dsv = mDevice->createTextureView(
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
        ComPtr<gfx::IResourceView> rtv =
            mDevice->createTextureView(colorBuffer.get(), colorBufferViewDesc);

        gfx::IFramebuffer::Desc framebufferDesc;
        framebufferDesc.renderTargetCount = 1;
        framebufferDesc.depthStencilView = dsv.get();
        framebufferDesc.renderTargetViews = rtv.readRef();
        framebufferDesc.layout = mFramebufferLayout;
        ComPtr<gfx::IFramebuffer> frameBuffer =
            mDevice->createFramebuffer(framebufferDesc);

        mFramebuffers.push_back(frameBuffer);
    }
}

void SampleApp::createComputeTexture() {
    int width = mSwapchain->getDesc().width;
    int height = mSwapchain->getDesc().height;
    ITextureResource::Desc resultTextureDesc = {};
    resultTextureDesc.type = IResource::Type::Texture2D;
    resultTextureDesc.numMipLevels = 1;
    resultTextureDesc.size.width = width;
    resultTextureDesc.size.height = height;
    resultTextureDesc.size.depth = 1;
    resultTextureDesc.defaultState = ResourceState::UnorderedAccess;
    resultTextureDesc.format = Format::R16G16B16A16_FLOAT;
    auto resource = mDevice->createTextureResource(resultTextureDesc);
    IResourceView::Desc resultUAVDesc = {};
    resultUAVDesc.format = resultTextureDesc.format;
    resultUAVDesc.type = IResourceView::Type::UnorderedAccess;
    auto textureUAV = mDevice->createTextureView(resource, resultUAVDesc);

    mComputeTexture.resource = resource;
    mComputeTexture.view = textureUAV;
}

void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob) {
    if (diagnosticsBlob != nullptr) {
        logError("{}", (const char*)diagnosticsBlob->getBufferPointer());
    }
}

Slang::ComPtr<gfx::IShaderProgram> SampleApp::createGraphicsShader() {
    ComPtr<slang::ISession> slangSession;
    slangSession = mDevice->getSlangSession();

    ComPtr<slang::IBlob> diagnosticsBlob;
    slang::IModule* module = slangSession->loadModule(
        "present.raster.slang", diagnosticsBlob.writeRef());
    diagnoseIfNeeded(diagnosticsBlob);
    VL_ASSERT(module != nullptr);

    ComPtr<slang::IEntryPoint> vertexEntryPoint;
    module->findEntryPointByName("vertexMain", vertexEntryPoint.writeRef());

    ComPtr<slang::IEntryPoint> fragmentEntryPoint;
    module->findEntryPointByName("fragmentMain", fragmentEntryPoint.writeRef());

    std::vector<slang::IComponentType*> componentTypes;
    componentTypes.push_back(module);

    int entryPointCount = 0;
    int vertexEntryPointIndex = entryPointCount++;
    componentTypes.push_back(vertexEntryPoint);

    int fragmentEntryPointIndex = entryPointCount++;
    componentTypes.push_back(fragmentEntryPoint);

    ComPtr<slang::IComponentType> linkedProgram;
    SlangResult result = slangSession->createCompositeComponentType(
        componentTypes.data(), componentTypes.size(), linkedProgram.writeRef(),
        diagnosticsBlob.writeRef());
    diagnoseIfNeeded(diagnosticsBlob);

    IShaderProgram::Desc programDesc = {};
    programDesc.slangGlobalScope = linkedProgram;
    return mDevice->createProgram(programDesc);
}

Slang::ComPtr<gfx::IShaderProgram> SampleApp::createComputeShader() {
    ComPtr<slang::ISession> slangSession;
    slangSession = mDevice->getSlangSession();

    ComPtr<slang::IBlob> diagnosticsBlob;
    slang::IModule* module =
        slangSession->loadModule("viz.cs.slang", diagnosticsBlob.writeRef());
    diagnoseIfNeeded(diagnosticsBlob);
    VL_ASSERT(module != nullptr);

    ComPtr<slang::IEntryPoint> computeEntryPoint;
    module->findEntryPointByName("main", computeEntryPoint.writeRef());

    std::vector<slang::IComponentType*> componentTypes{module,
                                                       computeEntryPoint};

    ComPtr<slang::IComponentType> linkedProgram;
    slangSession->createCompositeComponentType(
        componentTypes.data(), componentTypes.size(), linkedProgram.writeRef(),
        diagnosticsBlob.writeRef());
    diagnoseIfNeeded(diagnosticsBlob);

    IShaderProgram::Desc programDesc = {};
    programDesc.slangGlobalScope = linkedProgram;
    return mDevice->createProgram(programDesc);
}

void SampleApp::handleMessage(DebugMessageType type, DebugMessageSource source,
                              const char* message) {
    std::string sourceStr = source == DebugMessageSource::Driver  ? "Driver"
                            : source == DebugMessageSource::Layer ? "Layer"
                                                                  : "Slang";
    auto msg = fmt::format("[{}] {}", sourceStr, message);
    switch (type) {
        case gfx::DebugMessageType::Info:
            logInfo(msg);
            break;
        case gfx::DebugMessageType::Warning:
            logWarning(msg);
            break;
        case gfx::DebugMessageType::Error:
            logError(msg);
            break;
    }
}

void SampleApp::handleRenderFrame() {
    int framebufferIndex = mSwapchain->acquireNextImage();

    mTransientHeaps[framebufferIndex]->synchronizeAndReset();
    executeRenderFrame(framebufferIndex);
    mTransientHeaps[framebufferIndex]->finish();
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

void SampleApp::beginLoop() { mpWindow->msgLoop(); }

void SampleApp::executeRenderFrame(int framebufferIndex) {
    int width = mSwapchain->getDesc().width;
    int height = mSwapchain->getDesc().height;
    {
        ComPtr<ICommandBuffer> commandBuffer =
            mTransientHeaps[framebufferIndex]->createCommandBuffer();
        auto renderEncoder = commandBuffer->encodeComputeCommands();

        auto rootObject = renderEncoder->bindPipeline(mComputePipelineState);

        auto deviceInfo = mDevice->getDeviceInfo();

        // ShaderVar rootVar(rootObject);
        // float4x4 proj;
        // std::memcpy(&proj, deviceInfo.identityProjectionMatrix,
        //             sizeof(float) * 16);
        // proj *= 0.01f;
        // rootVar["Uniforms"]["modelViewProjection"] = proj;

        renderEncoder->dispatchCompute(width, height, 1);
        renderEncoder->endEncoding();
        commandBuffer->close();
        mQueue->executeCommandBuffer(commandBuffer);
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

        auto deviceInfo = mDevice->getDeviceInfo();

        ShaderVar rootVar(rootObject);
        float4x4 proj;
        std::memcpy(&proj, deviceInfo.identityProjectionMatrix,
                    sizeof(float) * 16);
        proj *= 0.01f;
        rootVar["Uniforms"]["modelViewProjection"] = proj;

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