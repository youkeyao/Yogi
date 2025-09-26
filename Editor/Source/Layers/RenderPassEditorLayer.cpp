#include "Layers/RenderPassEditorLayer.h"

#include "Registry/AssetRegistry.h"

#include <imgui.h>

namespace Yogi
{

const char* TextureFormatStr[]   = { "R8G8B8_UNORM",   "R8G8B8_SRGB",   "R8G8B8A8_UNORM",     "R8G8B8A8_SRGB",
                                     "B8G8R8A8_UNORM", "B8G8R8A8_SRGB", "R32G32B32A32_FLOAT", "R32G32B32_FLOAT",
                                     "R32_FLOAT",      "D32_FLOAT",     "D24_UNORM_S8_UINT",  "NONE" };
const char* AttachmentUsageStr[] = { "Color", "DepthStencil", "Resolve", "Present", "ShaderRead" };
const char* LoadOpStr[]          = { "Load", "Clear", "DontCare" };
const char* StoreOpStr[]         = { "Store", "DontCare" };

RenderPassEditorLayer::RenderPassEditorLayer() : Layer("RenderPass Editor Layer") {}

void RenderPassEditorLayer::OnUpdate(Timestep ts)
{
    ImGui::Begin("RenderPass Editor");
    float cursorY = ImGui::GetCursorPosY();

    if (m_renderPass)
    {
        ImGui::LabelText("", "%s", m_key.c_str());
        ImGui::Separator();
        auto renderPassDesc = m_renderPass->GetDesc();
        for (auto& colorAttachment : renderPassDesc.ColorAttachments)
        {
            OnAttachment(colorAttachment);
        }
        OnAttachment(renderPassDesc.DepthAttachment);
        ImGui::LabelText("", "%d", (uint8_t)renderPassDesc.NumSamples);
    }

    // blank space
    ImGui::SetCursorPosY(cursorY);
    ImVec2 viewportRegionMin = ImGui::GetWindowContentRegionMin();
    ImVec2 viewportRegionMax = ImGui::GetWindowContentRegionMax();
    ImGui::InvisibleButton("blank",
                           { std::max(viewportRegionMax.x - viewportRegionMin.x, 1.0f),
                             std::max(viewportRegionMax.y - viewportRegionMin.y, 1.0f) });
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ContentBrowserItem"))
        {
            const char*           path  = (const char*)payload->Data;
            std::filesystem::path fpath = path;
            if (fpath.extension().string() == ".rp")
            {
                m_key        = path;
                m_renderPass = AssetManager::GetAsset<IRenderPass>(m_key);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void RenderPassEditorLayer::OnEvent(Event& event) {}

// --------------------------------------------------------------------------

void RenderPassEditorLayer::OnAttachment(AttachmentDesc& attachment)
{
    ImGui::LabelText("", "%s", TextureFormatStr[(uint8_t)attachment.Format]);
    ImGui::LabelText("", "%s", AttachmentUsageStr[(uint8_t)attachment.Usage]);
    ImGui::LabelText("", "%s", LoadOpStr[(uint8_t)attachment.ColorLoadOp]);
    ImGui::LabelText("", "%s", StoreOpStr[(uint8_t)attachment.ColorStoreOp]);
}

} // namespace Yogi
