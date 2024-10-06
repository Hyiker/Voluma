#include "SampleApp.h"

#include <fmt/format.h>
#include <slang-com-ptr.h>
#include <slang-gfx.h>
#include <slang.h>

#include <cstring>

#include "Core/Math.h"
#include "Core/Program/ShaderVar.h"
#include "Error.h"
#include "Utils/Logger.h"
namespace Voluma {

using namespace gfx;
struct Vertex {
    float position[3];
    float color[3];
};

static const int kVertexCount = 3;
static const Vertex kVertexData[kVertexCount] = {
    {{0, 0, 0.5}, {1, 0, 0}},
    {{0, 1, 0.5}, {0, 0, 1}},
    {{1, 0, 0.5}, {0, 1, 0}},
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
        gfx::WindowHandle::FromHwnd(mpWindow->getNativeHandle()));
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
        {"POSITION", 0, Format::R32G32B32_FLOAT, offsetof(Vertex, position)},
        {"COLOR", 0, Format::R32G32B32_FLOAT, offsetof(Vertex, color)},
    };
    auto inputLayout =
        mDevice->createInputLayout(sizeof(Vertex), &inputElements[0], 2);
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

    Slang::ComPtr<gfx::IShaderProgram> program;
    initShader(program.writeRef());

    // Create pipeline
    GraphicsPipelineStateDesc desc;
    desc.inputLayout = inputLayout;
    desc.program = program;
    desc.framebufferLayout = mFramebufferLayout;
    mPipelineState = mDevice->createGraphicsPipelineState(desc);
    VL_ASSERT(mPipelineState != nullptr);
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

        gfx::IResourceView::Desc depthBufferViewDesc;
        memset(&depthBufferViewDesc, 0, sizeof(depthBufferViewDesc));
        depthBufferViewDesc.format = gfx::Format::D32_FLOAT;
        depthBufferViewDesc.renderTarget.shape =
            gfx::IResource::Type::Texture2D;
        depthBufferViewDesc.type = gfx::IResourceView::Type::DepthStencil;
        ComPtr<gfx::IResourceView> dsv = mDevice->createTextureView(
            depthBufferResource.get(), depthBufferViewDesc);

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

void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob) {
    if (diagnosticsBlob != nullptr) {
        logError("{}", (const char*)diagnosticsBlob->getBufferPointer());
    }
}

void SampleApp::initShader(gfx::IShaderProgram** pShader) {
    ComPtr<slang::ISession> slangSession;
    slangSession = mDevice->getSlangSession();

    ComPtr<slang::IBlob> diagnosticsBlob;
    slang::IModule* module =
        slangSession->loadModule("triangle.slang", diagnosticsBlob.writeRef());
    diagnoseIfNeeded(diagnosticsBlob);
    if (!module) {
        logFatal("");
    }

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

    gfx::IShaderProgram::Desc programDesc = {};
    programDesc.slangGlobalScope = linkedProgram;
    mDevice->createProgram(programDesc, pShader);
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

void SampleApp::beginLoop() { mpWindow->msgLoop(); }

void SampleApp::executeRenderFrame(int framebufferIndex) {
    ComPtr<ICommandBuffer> commandBuffer =
        mTransientHeaps[framebufferIndex]->createCommandBuffer();
    auto renderEncoder = commandBuffer->encodeRenderCommands(
        mRenderPass, mFramebuffers[framebufferIndex]);

    gfx::Viewport viewport = {};
    viewport.maxZ = 1.0f;
    viewport.extentX = mSwapchain->getDesc().width;
    viewport.extentY = mSwapchain->getDesc().height;
    renderEncoder->setViewportAndScissor(viewport);

    // In order to bind shader parameters to the pipeline, we need
    // to know how those parameters were assigned to
    // locations/bindings/registers for the target graphics API.
    //
    // The Slang compiler assigns locations to parameters in a deterministic
    // fashion, so it is possible for a programmer to hard-code locations
    // into their application code that will match up with their shaders.
    //
    // Hard-coding of locations can become intractable as an application needs
    // to support more different target platforms and graphics APIs, as well
    // as more shaders with different specialized variants.
    //
    // Rather than rely on hard-coded locations, our examples will make use of
    // reflection information provided by the Slang compiler (see
    // `programLayout` above), and our example graphics API layer will translate
    // that reflection information into a layout for a "root shader object."
    //
    // The root object will store values/bindings for all of the parameters in
    // the `IShaderProgram` used to create the pipeline state. At a conceptual
    // level we can think of `rootObject` as representing the "global scope" of
    // the shader program that was loaded; it has entries for each global shader
    // parameter that was declared.
    //
    // Readers who are familiar with D3D12 or Vulkan might think of this root
    // layout as being similar in spirit to a "root signature" or "pipeline
    // layout."
    //
    // We start parameter binding by binding the pipeline state in command
    // encoder. This method will return a transient root shader object for us to
    // write our shader parameters into.
    //
    auto rootObject = renderEncoder->bindPipeline(mPipelineState);

    // We will update the model-view-projection matrix that is passed
    // into the shader code via the `Uniforms` buffer on a per-frame
    // basis, even though the data that is loaded does not change
    // per-frame (we always use an identity matrix).
    //
    auto deviceInfo = mDevice->getDeviceInfo();

    // We know that `rootObject` is a root shader object created
    // from our program, and that it is set up to hold values for
    // all the parameter of that program. In order to actually
    // set values, we need to be able to look up the location
    // of speciic parameter that we want to set.
    //
    // Our example graphics API layer supports this operation
    // with the idea of a *shader cursor* which can be thought
    // of as pointing "into" a particular shader object at
    // some location/offset. This design choice abstracts over
    // the many ways that different platforms and APIs represent
    // the necessary offset information.
    //
    // We construct an initial shader cursor that points at the
    // entire shader program. You can think of this as akin to
    // a diretory path of `/` for the root directory in a file
    // system.
    //
    //
    // Next, we use a convenience overload of `operator[]` to
    // navigate from the root cursor down to the parameter we
    // want to set.
    //
    // The operation `rootCursor["Uniforms"]` looks up the
    // offset/location of the global shader parameter `Uniforms`
    // (which is a uniform/constant buffer), and the subsequent
    // `["modelViewProjection"]` step navigates from there down
    // to the member named `modelViewProjection` in that buffer.
    //
    // Once we have formed a cursor that "points" at the
    // model-view projection matrix, we can set its data directly.
    //

    ShaderVar rootVar(rootObject);
    float4x4 proj;
    std::memcpy(&proj, deviceInfo.identityProjectionMatrix, sizeof(float) * 16);
    proj *= 0.01f;
    rootVar["Uniforms"]["modelViewProjection"] = proj;
    //
    // Some readers might be concerned about the performance o
    // the above operations because of the use of strings. For
    // those readers, here are two things to note:
    //
    // * While these `operator[]` steps do need to perform string
    //   comparisons, they do *not* make copies of the strings or
    //   perform any heap allocation.
    //
    // * There are other overloads of `operator[]` that use the
    //   *index* of a parameter/field instead of its name, and those
    //   operations have fixed/constant overhead and perform no
    //   string comparisons. The indices used are independent of
    //   the target platform and graphics API, and can thus be
    //   hard-coded even in cross-platform code.
    //

    // We also need to set up a few pieces of fixed-function pipeline
    // state that are not bound by the pipeline state above.
    //
    renderEncoder->setVertexBuffer(0, mVertexBuffer);
    renderEncoder->setPrimitiveTopology(PrimitiveTopology::TriangleList);

    // Finally, we are ready to issue a draw call for a single triangle.
    //
    renderEncoder->draw(3);
    renderEncoder->endEncoding();
    commandBuffer->close();
    mQueue->executeCommandBuffer(commandBuffer);

    mSwapchain->present();
}
SampleApp::~SampleApp() {}

} // namespace Voluma