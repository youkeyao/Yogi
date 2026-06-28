#pragma once

#include <Yogi.h>
#include <imgui.h>

namespace Yogi
{

class ImGuiEndLayer : public Layer
{
public:
    ImGuiEndLayer();
    virtual ~ImGuiEndLayer();

    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

    void RenderDrawData(ImDrawData* drawData, ICommandBuffer* cmd, uint32_t fbWidth, uint32_t fbHeight);

    static WRef<IShaderResourceBinding> CreateImageBinding();

private:
    bool OnWindowResize(WindowResizeEvent& e);
    void WindowShutdown();

    void RendererInit();
    void RendererDraw();
    void RendererShutdown();

    void UpdateFontTexture(ImTextureData* tex);
    void EnsureBufferCapacity(WRef<IBuffer>& buffer, uint64_t& capacity, uint64_t needed, BufferUsage usage);

private:
    WRef<ICommandBuffer> m_commandBuffer;

    WRef<IPipeline> m_pipeline;
    Format          m_colorFormat = Format::NONE;

    WRef<IBuffer> m_vertexBuffer;
    WRef<IBuffer> m_indexBuffer;
    uint64_t      m_vertexCapacity = 0; // bytes
    uint64_t      m_indexCapacity  = 0; // bytes

    struct FontTexture
    {
        WRef<ITexture>               Texture;
        WRef<ITextureView>           View;
        WRef<IShaderResourceBinding> Srb;
    };
    std::unordered_map<ImTextureData*, FontTexture> m_fontTextures;
};

} // namespace Yogi
