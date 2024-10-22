#include "Gui.h"

#include <imgui.h>

#include "Core/Device.h"
#include "Core/Error.h"
#include "Core/Program/Program.h"
#include "Core/Program/ShaderVar.h"
#include "Core/Texture.h"
#include "Core/Window.h"
#include "Utils/Logger.h"
#include "slang-gfx.h"

using namespace gfx;

namespace Voluma {

Gui::Gui(Window* window, const Device::SharedPtr& pDevice,
         const ProgramManager::SharedPtr& programManager,
         Slang::ComPtr<gfx::ICommandQueue> queue,
         Slang::ComPtr<gfx::IFramebufferLayout> framebufferLayout)
    : mpDevice(pDevice), mpQueue(queue) {
    mpContext = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    auto gfxDevice = pDevice->getGfxDevice();

    // Let's do the initialization work required for our graphics API
    // abstraction layer, so that we can pipe all IMGUI rendering
    // through the same interface as other work.
    //

    auto program = programManager->createProgram(
        "Shaders/Gui.raster.slang",
        {{"vsMain", ShaderType::Vertex}, {"psMain", ShaderType::Pixel}});
    InputElementDesc inputElements[] = {
        {"POSITION", 0, Format::R32G32_FLOAT, offsetof(ImDrawVert, pos), 0},
        {"TEXCOORD", 0, Format::R32G32_FLOAT, offsetof(ImDrawVert, uv), 0},
        {"COLOR", 0, Format::R8G8B8A8_UNORM, offsetof(ImDrawVert, col), 0},
    };
    auto inputLayout = gfxDevice->createInputLayout(
        sizeof(ImDrawVert), &inputElements[0], SLANG_COUNT_OF(inputElements));

    TargetBlendDesc targetBlendDesc;
    targetBlendDesc.enableBlend = true;
    targetBlendDesc.color.srcFactor = BlendFactor::SrcAlpha;
    targetBlendDesc.color.dstFactor = BlendFactor::InvSrcAlpha;
    targetBlendDesc.alpha.srcFactor = BlendFactor::InvSrcAlpha;
    targetBlendDesc.alpha.dstFactor = BlendFactor::Zero;

    GraphicsPipelineStateDesc pipelineDesc;
    pipelineDesc.framebufferLayout = framebufferLayout;
    pipelineDesc.program = program;
    pipelineDesc.inputLayout = inputLayout;
    pipelineDesc.blend.targets[0] = targetBlendDesc;
    pipelineDesc.blend.targetCount = 1;
    pipelineDesc.rasterizer.cullMode = CullMode::None;

    // Set up the pieces of fixed-function state that we care about
    pipelineDesc.depthStencil.depthTestEnable = false;

    // TODO: need to set up blending state...

    mpPipelineState = gfxDevice->createGraphicsPipelineState(pipelineDesc);

    // Initialize the texture atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    {
        gfx::ITextureResource::Desc desc{};
        desc.type = IResource::Type::Texture2D;
        desc.numMipLevels = 1;
        desc.arraySize = 0;
        desc.size.width = width;
        desc.size.height = height;
        desc.size.depth = 1;
        desc.defaultState = ResourceState::ShaderResource;
        desc.allowedStates.add(ResourceState::ShaderResource,
                               ResourceState::CopyDestination);
        desc.format = Format::R8G8B8A8_UNORM;

        ITextureResource::SubresourceData initData;
        initData.data = pixels;
        initData.strideY = width * 4 * sizeof(unsigned char);

        gfx::IResourceView::Desc viewDesc{};
        viewDesc.format = desc.format;
        viewDesc.type = IResourceView::Type::ShaderResource;

        mpFontTex = mpDevice->createTexture(desc, viewDesc, &initData);
    }
    io.Fonts->SetTexID(
        static_cast<ImTextureID>(mpFontTex->getResource().get()));

    {
        ISamplerState::Desc desc;
        mpSamplerState = gfxDevice->createSamplerState(desc);
    }

    {
        IRenderPassLayout::Desc desc;
        desc.framebufferLayout = framebufferLayout;
        IRenderPassLayout::TargetAccessDesc colorAccess;
        desc.depthStencilAccess = nullptr;
        colorAccess.initialState = ResourceState::Present;
        colorAccess.finalState = ResourceState::Present;
        colorAccess.loadOp = IRenderPassLayout::TargetLoadOp::Load;
        colorAccess.storeOp = IRenderPassLayout::TargetStoreOp::Store;
        desc.renderTargetAccess = &colorAccess;
        desc.renderTargetCount = 1;
        mpRenderPass = gfxDevice->createRenderPassLayout(desc);
    }
    {
        gfx::ISamplerState::Desc samplerDesc = {};
        mpSampler = gfxDevice->createSamplerState(samplerDesc);
    }
}

void Gui::beginFrame() {
    ImGui::SetCurrentContext(mpContext);
    ImGui::NewFrame();
}

void Gui::endFrame(ITransientResourceHeap* transientHeap,
                   IFramebuffer* framebuffer) {
    ImGui::SetCurrentContext(mpContext);

    ImGui::Render();

    ImDrawData* pDrawData = ImGui::GetDrawData();
    uint32_t vertexCount = pDrawData->TotalVtxCount;
    uint32_t indexCount = pDrawData->TotalIdxCount;
    int commandListCount = pDrawData->CmdListsCount;

    if (!vertexCount) return;
    if (!indexCount) return;
    if (!commandListCount) return;

    auto cache = getBufferCache(vertexCount, indexCount);
    const auto& vertexBuffer = cache.vertexBuffer;
    const auto& indexBuffer = cache.indexBuffer;

    // Copy buffer data
    {
        ImDrawVert* pVertex;
        ImDrawIdx* pIndex;

        gfx::MemoryRange vertexRange{
            .offset = 0,
            .size = vertexCount * sizeof(ImDrawVert),
        };
        vertexBuffer->map(&vertexRange, (void**)&pVertex);

        gfx::MemoryRange indexRange{
            .offset = 0,
            .size = indexCount * sizeof(ImDrawIdx),
        };
        indexBuffer->map(&indexRange, (void**)&pIndex);

        for (int n = 0; n < commandListCount; ++n) {
            const ImDrawList* commandList = pDrawData->CmdLists[n];

            std::memcpy(pVertex, commandList->VtxBuffer.Data,
                        commandList->VtxBuffer.Size * sizeof(ImDrawVert));
            pVertex += commandList->VtxBuffer.Size;

            std::memcpy(pIndex, commandList->IdxBuffer.Data,
                        commandList->IdxBuffer.Size * sizeof(ImDrawIdx));
            pIndex += commandList->IdxBuffer.Size;
        }

        vertexBuffer->unmap(&vertexRange);
        indexBuffer->unmap(&indexRange);
    }

    auto cmdBuf = transientHeap->createCommandBuffer();

    gfx::Viewport viewport;
    viewport.originX = 0;
    viewport.originY = 0;
    viewport.extentX = pDrawData->DisplaySize.x;
    viewport.extentY = pDrawData->DisplaySize.y;
    viewport.minZ = 0;
    viewport.maxZ = 1;

    auto renderEncoder =
        cmdBuf->encodeRenderCommands(mpRenderPass, framebuffer);

    renderEncoder->bindPipeline(mpPipelineState);

    renderEncoder->setVertexBuffer(0, vertexBuffer);
    renderEncoder->setIndexBuffer(indexBuffer, sizeof(ImDrawIdx) == 2
                                                   ? Format::R16_UINT
                                                   : Format::R32_UINT);
    renderEncoder->setViewportAndScissor(viewport);
    renderEncoder->setPrimitiveTopology(PrimitiveTopology::TriangleList);

    auto rootObject = renderEncoder->bindPipeline(mpPipelineState);
    ShaderVar rootVar(rootObject);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = (float)1920;
    io.DisplaySize.y = (float)1080;

    rootVar["gFont"] = *mpFontTex;
    rootVar["scale"] = 2.0f / float2(io.DisplaySize.x, -io.DisplaySize.y);
    rootVar["offset"] = float2(-1.0f, 1.0f);
    rootVar["gSampler"] = mpSampler.get();

    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    ImVec2 pos = pDrawData->DisplayPos;
    for (int n = 0; n < commandListCount; ++n) {
        auto commandList = pDrawData->CmdLists[n];
        auto commandCount = commandList->CmdBuffer.Size;
        for (int cmdIdx = 0; cmdIdx < commandCount; cmdIdx++) {
            auto command = &commandList->CmdBuffer[cmdIdx];
            if (auto userCallback = command->UserCallback) {
                userCallback(commandList, command);
            } else {
                ScissorRect rect = {(int32_t)(command->ClipRect.x - pos.x),
                                    (int32_t)(command->ClipRect.y - pos.y),
                                    (int32_t)(command->ClipRect.z - pos.x),
                                    (int32_t)(command->ClipRect.w - pos.y)};
                renderEncoder->setScissorRects(1, &rect);

                // TODO: set parameter into root shader object.
                renderEncoder->drawIndexed(command->ElemCount,
                                           (uint32_t)indexOffset,
                                           (uint32_t)vertexOffset);
            }
            indexOffset += command->ElemCount;
        }
        vertexOffset += commandList->VtxBuffer.Size;
    }
    renderEncoder->endEncoding();
    cmdBuf->close();
    mpQueue->executeCommandBuffer(cmdBuf);
}

bool Gui::onMouseEvent(const MouseEvent& mouseEvent) {
    ImGui::SetCurrentContext(mpContext);
    ImGuiIO& io = ImGui::GetIO();

    switch (mouseEvent.type) {
        case MouseEvent::Type::ButtonUp:
        case MouseEvent::Type::ButtonDown:
            io.AddMouseButtonEvent(
                static_cast<int>(mouseEvent.button),
                mouseEvent.type == MouseEvent::Type::ButtonDown);
            break;
        case MouseEvent::Type::Move:
            io.AddMousePosEvent(mouseEvent.pos.x * 1920,
                                mouseEvent.pos.y * 1080);
            break;
        case MouseEvent::Type::Wheel:
            io.AddMouseWheelEvent(mouseEvent.wheelDelta.x,
                                  mouseEvent.wheelDelta.y);
            break;
        default:
            break;
    }
    return io.WantCaptureMouse;
}

Gui::~Gui() { ImGui::DestroyContext(mpContext); }

Gui::BufferCache Gui::getBufferCache(uint32_t vertexCount,
                                     uint32_t indexCount) {
    bool createVB = true;
    bool createIB = true;

    uint32_t requiredVbSize = vertexCount * sizeof(ImDrawVert);
    uint32_t requiredIbSize = indexCount * sizeof(ImDrawIdx);

    auto& cache = mRotateBufferCache[mCacheIndex];
    mCacheIndex = (mCacheIndex + 1) % kBufferCacheCount;

    createVB = cache.vertexBufferSize < requiredVbSize;
    createIB = cache.indexBufferSize < requiredIbSize;

    if (createVB) {
        if (cache.vertexBuffer != nullptr) {
            cache.vertexBuffer->Release();
        }

        gfx::IBufferResource::Desc vertexBufferDesc;
        vertexBufferDesc.type = IResource::Type::Buffer;
        vertexBufferDesc.memoryType = MemoryType::Upload;
        vertexBufferDesc.defaultState = ResourceState::VertexBuffer;
        vertexBufferDesc.allowedStates.add(ResourceState::VertexBuffer,
                                           gfx::ResourceState::CopyDestination);
        vertexBufferDesc.sizeInBytes = requiredVbSize + 1024 * 128;
        auto vertexBuffer =
            mpDevice->getGfxDevice()->createBufferResource(vertexBufferDesc);
        VL_ASSERT(vertexBuffer != nullptr);

        cache.vertexBuffer = vertexBuffer;
        cache.vertexBufferSize = vertexBufferDesc.sizeInBytes;
    }

    if (createIB) {
        if (cache.indexBuffer != nullptr) {
            cache.indexBuffer->Release();
        }
        gfx::IBufferResource::Desc indexBufferDesc;
        indexBufferDesc.type = IResource::Type::Buffer;
        indexBufferDesc.memoryType = MemoryType::Upload;
        indexBufferDesc.sizeInBytes = requiredIbSize + 1024;
        indexBufferDesc.allowedStates.add(ResourceState::IndexBuffer,
                                          gfx::ResourceState::CopyDestination);
        indexBufferDesc.defaultState = ResourceState::IndexBuffer;
        auto indexBuffer =
            mpDevice->getGfxDevice()->createBufferResource(indexBufferDesc);
        VL_ASSERT(indexBuffer != nullptr);

        cache.indexBuffer = indexBuffer;
        cache.indexBufferSize = indexBufferDesc.sizeInBytes;
    }

    return cache;
}

} // namespace Voluma
