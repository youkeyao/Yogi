#include "Layers/ImGuiEndLayer.h"
#include "Utils/ImGuiBackends.h"

namespace Yogi
{

namespace
{

struct ImGuiPushConstant
{
    float Scale[2];
    float Translate[2];
};

class ImGuiViewportWindow : public Window
{
public:
    ImGuiViewportWindow(void* nativeHandle, uint32_t width, uint32_t height) :
        m_native(nativeHandle),
        m_width(width),
        m_height(height)
    {}

    void     OnUpdate() override {}
    void     SetEventCallback(const EventCallbackFn&) override {}
    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }
    void*    GetNativeWindow() const override { return m_native; }

    void SetSize(uint32_t width, uint32_t height)
    {
        m_width  = width;
        m_height = height;
    }

private:
    void*    m_native;
    uint32_t m_width;
    uint32_t m_height;
};

// ---------------------------------------------------------------------------
// Multi-viewport support
// ---------------------------------------------------------------------------
struct ImGuiViewportData
{
    Owner<ImGuiViewportWindow> Window;
    Owner<ISwapChain>          SwapChain;
    WRef<ImGuiEndLayer>        Layer;
};

void Viewport_CreateWindow(ImGuiViewport* viewport)
{
    auto* vd = IM_NEW(ImGuiViewportData)();

    const uint32_t w = viewport->Size.x > 0 ? static_cast<uint32_t>(viewport->Size.x) : 1;
    const uint32_t h = viewport->Size.y > 0 ? static_cast<uint32_t>(viewport->Size.y) : 1;
    vd->Window       = Owner<ImGuiViewportWindow>::Create(viewport->PlatformHandle, w, h);

    SwapChainDesc desc{};
    desc.Width       = w;
    desc.Height      = h;
    desc.ColorFormat = Application::GetInstance().GetSwapChain()->GetColorFormat();
    desc.NumSamples  = SampleCountFlagBits::Count1;
    desc.Window      = vd->Window.Get();

    vd->SwapChain = Owner<ISwapChain>::Create(desc);
    vd->Layer     = WRef<ImGuiEndLayer>::Cast(Application::GetInstance().AcquireLayer("ImGuiEndLayer"));

    viewport->RendererUserData = vd;
}

void Viewport_DestroyWindow(ImGuiViewport* viewport)
{
    if (auto* vd = static_cast<ImGuiViewportData*>(viewport->RendererUserData))
    {
        Application::GetInstance().GetContext()->WaitIdle();
        IM_DELETE(vd);
    }
    viewport->RendererUserData = nullptr;
}

void Viewport_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    auto* vd = static_cast<ImGuiViewportData*>(viewport->RendererUserData);
    if (!vd || !vd->SwapChain)
        return;
    const uint32_t w = size.x > 0 ? static_cast<uint32_t>(size.x) : 1;
    const uint32_t h = size.y > 0 ? static_cast<uint32_t>(size.y) : 1;
    vd->Window->SetSize(w, h);
    vd->SwapChain->Resize(w, h);
}

void Viewport_RenderWindow(ImGuiViewport* viewport, void*)
{
    auto* vd = static_cast<ImGuiViewportData*>(viewport->RendererUserData);
    if (!vd || !vd->SwapChain)
        return;

    ISwapChain* swapChain = vd->SwapChain.Get();
    swapChain->AcquireNextImage();

    WRef<ITextureView>   target = swapChain->AcquireCurrentTarget();
    WRef<ICommandBuffer> cmdRef = swapChain->AcquireCurrentCommandBuffer();
    ICommandBuffer*      cmd    = cmdRef.Get();
    const ITexture*      tex    = target->GetTexture();

    cmd->Begin();
    cmd->Barrier(BarrierDesc{
        .TextureView = target.Get(),
        .BeforeState = ResourceState::Undefined,
        .AfterState  = ResourceState::ColorAttachment,
    });
    {
        RenderingDesc rdesc{};
        rdesc.Width   = tex->GetWidth();
        rdesc.Height  = tex->GetHeight();
        rdesc.Samples = SampleCountFlagBits::Count1;
        RenderingAttachment color{};
        color.View              = target.Get();
        color.LoadAction        = (viewport->Flags & ImGuiViewportFlags_NoRendererClear) ? LoadOp::Load : LoadOp::Clear;
        color.StoreAction       = StoreOp::Store;
        color.ClearVal.Color[0] = 0.0f;
        color.ClearVal.Color[1] = 0.0f;
        color.ClearVal.Color[2] = 0.0f;
        color.ClearVal.Color[3] = 1.0f;
        rdesc.ColorAttachments.push_back(color);
        cmd->BeginRendering(rdesc);
    }

    vd->Layer->RenderDrawData(viewport->DrawData, cmd, tex->GetWidth(), tex->GetHeight());

    cmd->EndRendering();
    cmd->Barrier(BarrierDesc{
        .TextureView = target.Get(),
        .BeforeState = ResourceState::ColorAttachment,
        .AfterState  = ResourceState::Present,
    });
    cmd->End();
    cmd->Submit();
}

void Viewport_SwapBuffers(ImGuiViewport* viewport, void*)
{
    if (auto* vd = static_cast<ImGuiViewportData*>(viewport->RendererUserData))
        if (vd->SwapChain)
            vd->SwapChain->Present();
}

void RegisterMultiViewport()
{
    ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    ImGuiPlatformIO& platformIO       = ImGui::GetPlatformIO();
    platformIO.Renderer_CreateWindow  = Viewport_CreateWindow;
    platformIO.Renderer_DestroyWindow = Viewport_DestroyWindow;
    platformIO.Renderer_SetWindowSize = Viewport_SetWindowSize;
    platformIO.Renderer_RenderWindow  = Viewport_RenderWindow;
    platformIO.Renderer_SwapBuffers   = Viewport_SwapBuffers;
}

void UnregisterMultiViewport()
{
    ImGui::DestroyPlatformWindows();
    ImGui::GetIO().BackendFlags &= ~ImGuiBackendFlags_RendererHasViewports;

    ImGuiPlatformIO& platformIO       = ImGui::GetPlatformIO();
    platformIO.Renderer_CreateWindow  = nullptr;
    platformIO.Renderer_DestroyWindow = nullptr;
    platformIO.Renderer_SetWindowSize = nullptr;
    platformIO.Renderer_RenderWindow  = nullptr;
    platformIO.Renderer_SwapBuffers   = nullptr;
}

} // namespace

// ===========================================================================
// Layer
// ===========================================================================
ImGuiEndLayer::ImGuiEndLayer() : Layer("ImGuiEndLayer")
{
    RendererInit();
}

ImGuiEndLayer::~ImGuiEndLayer()
{
    m_commandBuffer->Wait();
    m_commandBuffer = nullptr;
    WindowShutdown();
    RendererShutdown();
}

void ImGuiEndLayer::OnUpdate(Timestep ts)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::End();
    ImGui::Render();
    RendererDraw();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void ImGuiEndLayer::OnEvent(Event& event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowResizeEvent>(YG_BIND_FN(ImGuiEndLayer::OnWindowResize));
}

bool ImGuiEndLayer::OnWindowResize(WindowResizeEvent& e)
{
    m_commandBuffer->Wait();
    return false;
}

void ImGuiEndLayer::WindowShutdown()
{
#ifdef YG_WINDOW_GLFW
    ImGui_ImplGlfw_Shutdown();
#endif
}

WRef<IShaderResourceBinding> ImGuiEndLayer::CreateImageBinding()
{
    SamplerDesc sampler{};
    sampler.MagFilter = Filter::Linear;
    sampler.MinFilter = Filter::Linear;
    sampler.MipMode   = MipmapMode::Linear;
    sampler.AddressU  = SamplerAddressMode::ClampToEdge;
    sampler.AddressV  = SamplerAddressMode::ClampToEdge;
    sampler.AddressW  = SamplerAddressMode::ClampToEdge;

    return ResourceManager::CreateResource<IShaderResourceBinding>(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::Sampler, ShaderStage::Fragment },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::SampledTexture, ShaderStage::Fragment } },
        std::vector<ImmutableSamplerBindingDesc>{
            ImmutableSamplerBindingDesc{ 0, 1, ShaderStage::Fragment, sampler } });
}

void ImGuiEndLayer::UpdateFontTexture(ImTextureData* tex)
{
    if (tex->Status == ImTextureStatus_WantCreate)
    {
        YG_CORE_ASSERT(tex->Format == ImTextureFormat_RGBA32 && tex->BytesPerPixel == 4,
                       "ImGui backend: only RGBA32 atlas textures are supported");

        FontTexture ft;
        TextureDesc desc{};
        desc.Width      = static_cast<uint32_t>(tex->Width);
        desc.Height     = static_cast<uint32_t>(tex->Height);
        desc.MipLevels  = 1;
        desc.Format     = Format::R8G8B8A8_UNORM;
        desc.UsageFlags = TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst;

        ft.Texture = ResourceManager::CreateResource<ITexture>(desc);
        ft.View    = ResourceManager::CreateResource<ITextureView>(ft.Texture, TextureViewDesc{});
        ft.View->SetData(tex->GetPixels(), static_cast<uint32_t>(tex->GetSizeInBytes()));

        ft.Srb = CreateImageBinding();
        ft.Srb->BindTextureView(ft.View.Get(), 1, 0);

        IShaderResourceBinding* srbPtr = ft.Srb.Get();
        m_fontTextures[tex]            = std::move(ft);

        tex->SetTexID((ImTextureID)(intptr_t)srbPtr);
        tex->BackendUserData = tex; // marker; lookup is by ImTextureData* key
        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantUpdates)
    {
        auto it = m_fontTextures.find(tex);
        if (it != m_fontTextures.end() && it->second.View)
            it->second.View->SetData(tex->GetPixels(), static_cast<uint32_t>(tex->GetSizeInBytes()));
        tex->SetStatus(ImTextureStatus_OK);
    }
    else if (tex->Status == ImTextureStatus_WantDestroy)
    {
        m_fontTextures.erase(tex);
        tex->SetTexID(ImTextureID_Invalid);
        tex->BackendUserData = nullptr;
        tex->SetStatus(ImTextureStatus_Destroyed);
    }
}

void ImGuiEndLayer::EnsureBufferCapacity(WRef<IBuffer>& buffer, uint64_t& capacity, uint64_t needed, BufferUsage usage)
{
    if (buffer.Get() && capacity >= needed)
        return;
    uint64_t newCap = needed + needed / 2;
    if (newCap < 4096)
        newCap = 4096;
    buffer   = ResourceManager::CreateResource<IBuffer>(BufferDesc{ newCap, usage });
    capacity = newCap;
}

void ImGuiEndLayer::RendererInit()
{
    ImGuiIO& io            = ImGui::GetIO();
    io.BackendRendererName = "Yogi_RHI";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    m_colorFormat = Application::GetInstance().GetSwapChain()->GetColorFormat();

    WRef<ShaderDesc> vs = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/ImGui/ImGui.vs.slang");
    WRef<ShaderDesc> fs = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/ImGui/ImGui.fs.slang");
    YG_CORE_ASSERT(vs.Get() && fs.Get(), "ImGui backend: shader load failed");

    WRef<IShaderResourceBinding> bindingTemplate = CreateImageBinding();

    PipelineDesc desc{};
    desc.Type            = PipelineType::Graphics;
    desc.Shaders         = { vs.Get(), fs.Get() };
    desc.ResourceBinding = bindingTemplate.Get();
    desc.VertexLayout    = {
        VertexAttribute{ "POSITION", offsetof(ImDrawVert, pos), sizeof(ImVec2), Format::R32G32_FLOAT },
        VertexAttribute{ "TEXCOORD", offsetof(ImDrawVert, uv), sizeof(ImVec2), Format::R32G32_FLOAT },
        VertexAttribute{ "COLOR", offsetof(ImDrawVert, col), sizeof(ImU32), Format::R8G8B8A8_UNORM },
    };
    desc.PushConstantRanges = { PushConstantRange{
        ShaderStage::Vertex, 0, static_cast<uint32_t>(sizeof(ImGuiPushConstant)) } };
    desc.ColorTargets       = { ColorTargetDesc{
        .Format      = m_colorFormat,
        .ColorBlend  = { BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha, BlendOp::Add },
        .AlphaBlend  = { BlendFactor::One, BlendFactor::OneMinusSrcAlpha, BlendOp::Add },
        .EnableBlend = true,
        .WriteMask   = ColorWriteMask::All,
    } };
    desc.DepthFormat        = Format::NONE;
    desc.Samples            = SampleCountFlagBits::Count1;
    desc.Topology           = PrimitiveTopology::TriangleList;
    desc.Cull               = CullMode::None;
    desc.DepthTestEnable    = false;
    desc.DepthWriteEnable   = false;

    m_pipeline = ResourceManager::CreateResource<IPipeline>(desc);

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        RegisterMultiViewport();

    m_commandBuffer = ResourceManager::CreateResource<ICommandBuffer>(
        CommandBufferDesc{ CommandBufferUsage::OneTimeSubmit, SubmitQueue::Graphics });
}

void ImGuiEndLayer::RendererDraw()
{
    ImDrawData*        mainDrawData  = ImGui::GetDrawData();
    auto               swapChain     = Application::GetInstance().GetSwapChain();
    WRef<ITextureView> currentTarget = swapChain->AcquireCurrentTarget();
    const ITexture*    targetTex     = currentTarget->GetTexture();

    m_commandBuffer->Begin();

    m_commandBuffer->Barrier(BarrierDesc{
        .TextureView = currentTarget.Get(),
        .BeforeState = ResourceState::ColorAttachment,
        .AfterState  = ResourceState::ColorAttachment,
    });
    {
        RenderingDesc rdesc{};
        rdesc.Width   = targetTex->GetWidth();
        rdesc.Height  = targetTex->GetHeight();
        rdesc.Samples = SampleCountFlagBits::Count1;
        RenderingAttachment color{};
        color.View        = currentTarget.Get();
        color.LoadAction  = LoadOp::Load;
        color.StoreAction = StoreOp::Store;
        rdesc.ColorAttachments.push_back(color);
        m_commandBuffer->BeginRendering(rdesc);
    }

    RenderDrawData(mainDrawData, m_commandBuffer.Get(), targetTex->GetWidth(), targetTex->GetHeight());

    m_commandBuffer->EndRendering();

    m_commandBuffer->Barrier(BarrierDesc{
        .TextureView = currentTarget.Get(),
        .BeforeState = ResourceState::ColorAttachment,
        .AfterState  = ResourceState::Present,
    });

    m_commandBuffer->End();
    m_commandBuffer->Submit();
    m_commandBuffer->Wait();
}

void ImGuiEndLayer::RendererShutdown()
{
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        UnregisterMultiViewport();

    m_fontTextures.clear();
    m_vertexBuffer = nullptr;
    m_indexBuffer  = nullptr;
    m_pipeline     = nullptr;
}

void ImGuiEndLayer::RenderDrawData(ImDrawData* drawData, ICommandBuffer* cmd, uint32_t fbWidth, uint32_t fbHeight)
{
    if (!drawData)
        return;

    bool uploaded = false;
    if (drawData->Textures != nullptr)
    {
        for (ImTextureData* tex : *drawData->Textures)
        {
            if (tex->Status != ImTextureStatus_OK)
            {
                UpdateFontTexture(tex);
                uploaded = true;
            }
        }
    }
    if (uploaded)
        Application::GetInstance().GetContext()->WaitIdle();

    if (drawData->TotalVtxCount <= 0 || fbWidth == 0 || fbHeight == 0)
        return;

    const uint64_t vtxBytes = static_cast<uint64_t>(drawData->TotalVtxCount) * sizeof(ImDrawVert);
    const uint64_t idxBytes = static_cast<uint64_t>(drawData->TotalIdxCount) * sizeof(ImDrawIdx);
    EnsureBufferCapacity(m_vertexBuffer, m_vertexCapacity, vtxBytes, BufferUsage::Vertex | BufferUsage::Staging);
    EnsureBufferCapacity(m_indexBuffer, m_indexCapacity, idxBytes, BufferUsage::Index | BufferUsage::Staging);

    auto* vtxDst = static_cast<ImDrawVert*>(m_vertexBuffer->GetMappedPtr());
    auto* idxDst = static_cast<ImDrawIdx*>(m_indexBuffer->GetMappedPtr());
    for (const ImDrawList* list : drawData->CmdLists)
    {
        memcpy(vtxDst, list->VtxBuffer.Data, list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, list->IdxBuffer.Data, list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += list->VtxBuffer.Size;
        idxDst += list->IdxBuffer.Size;
    }

    cmd->SetPipeline(m_pipeline.Get());
    cmd->SetVertexBuffer(m_vertexBuffer.Get(), 0);
    cmd->SetIndexBuffer(m_indexBuffer.Get(), 0);
    cmd->SetViewport(
        { 0.0f, static_cast<float>(fbHeight), static_cast<float>(fbWidth), -static_cast<float>(fbHeight) });

    ImGuiPushConstant pc{};
    pc.Scale[0]     = 2.0f / drawData->DisplaySize.x;
    pc.Scale[1]     = 2.0f / drawData->DisplaySize.y;
    pc.Translate[0] = -1.0f - drawData->DisplayPos.x * pc.Scale[0];
    pc.Translate[1] = -1.0f - drawData->DisplayPos.y * pc.Scale[1];
    cmd->SetPushConstants(m_pipeline.Get(), ShaderStage::Vertex, 0, sizeof(pc), &pc);

    const ImVec2 clipOff   = drawData->DisplayPos;
    const ImVec2 clipScale = drawData->FramebufferScale;

    int                     globalVtxOffset = 0;
    int                     globalIdxOffset = 0;
    IShaderResourceBinding* lastSrb         = nullptr;
    for (const ImDrawList* list : drawData->CmdLists)
    {
        for (int i = 0; i < list->CmdBuffer.Size; ++i)
        {
            const ImDrawCmd& pcmd = list->CmdBuffer[i];
            if (pcmd.UserCallback != nullptr)
            {
                if (pcmd.UserCallback != ImDrawCallback_ResetRenderState)
                    pcmd.UserCallback(list, &pcmd);
                lastSrb = nullptr;
                continue;
            }

            ImVec2 clipMin((pcmd.ClipRect.x - clipOff.x) * clipScale.x, (pcmd.ClipRect.y - clipOff.y) * clipScale.y);
            ImVec2 clipMax((pcmd.ClipRect.z - clipOff.x) * clipScale.x, (pcmd.ClipRect.w - clipOff.y) * clipScale.y);
            if (clipMin.x < 0.0f)
                clipMin.x = 0.0f;
            if (clipMin.y < 0.0f)
                clipMin.y = 0.0f;
            if (clipMax.x > (float)fbWidth)
                clipMax.x = (float)fbWidth;
            if (clipMax.y > (float)fbHeight)
                clipMax.y = (float)fbHeight;
            if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                continue;

            cmd->SetScissor(Scissor{ static_cast<int32_t>(clipMin.x),
                                     static_cast<int32_t>(clipMin.y),
                                     static_cast<uint32_t>(clipMax.x - clipMin.x),
                                     static_cast<uint32_t>(clipMax.y - clipMin.y) });

            auto* srb = reinterpret_cast<IShaderResourceBinding*>(static_cast<intptr_t>(pcmd.GetTexID()));
            if (srb && srb != lastSrb)
            {
                cmd->SetShaderResourceBinding(srb);
                lastSrb = srb;
            }
            if (!srb)
                continue;

            cmd->DrawIndexed(pcmd.ElemCount,
                             1,
                             pcmd.IdxOffset + globalIdxOffset,
                             static_cast<int32_t>(pcmd.VtxOffset) + globalVtxOffset,
                             0);
        }
        globalIdxOffset += list->IdxBuffer.Size;
        globalVtxOffset += list->VtxBuffer.Size;
    }
}

} // namespace Yogi
