#include "Layers/RenderPassEditorLayer.h"
#include "Registry/AssetRegistry.h"
#include "Utils/ImGuiEnumCombo.h"

#include <imgui.h>

namespace Yogi
{

RenderPassEditorLayer::RenderPassEditorLayer() : Layer("RenderPass Editor Layer") {}

void RenderPassEditorLayer::OnUpdate(Timestep ts)
{
    ImGui::Begin("RenderPass Editor");
    float cursorY = ImGui::GetCursorPosY();

    if (m_renderPass)
    {
        ImGui::TextUnformatted(m_key.c_str());
        ImGui::Separator();
        auto               renderPassDesc = m_renderPass->GetDesc();
        bool               changed        = false;
        ImGuiTreeNodeFlags treeNodeFlags  = ImGuiTreeNodeFlags_FramePadding;
        if (ImGui::TreeNodeEx("Color Attachments", treeNodeFlags))
        {
            std::vector<Yogi::AttachmentDesc>::iterator removeIt = renderPassDesc.ColorAttachments.end();
            for (int i = 0; i < renderPassDesc.ColorAttachments.size(); i++)
            {
                bool isOpened = ImGui::TreeNodeEx(("ColorAttachment " + std::to_string(i)).c_str(), treeNodeFlags);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Delete Attachment"))
                    {
                        changed |= true;
                        removeIt = renderPassDesc.ColorAttachments.begin() + i;
                    }
                    ImGui::EndPopup();
                }
                if (isOpened)
                {
                    changed |= ImGuiAttachment(renderPassDesc.ColorAttachments[i]);
                    ImGui::TreePop();
                }
            }
            if (removeIt != renderPassDesc.ColorAttachments.end())
            {
                renderPassDesc.ColorAttachments.erase(removeIt);
            }
            if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
            {
                renderPassDesc.ColorAttachments.push_back({
                    ITexture::Format::R8G8B8A8_UNORM,
                    AttachmentUsage::ShaderRead,
                    LoadOp::Clear,
                    StoreOp::Store,
                });
                changed |= true;
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Depth Attachment", treeNodeFlags))
        {
            changed |= ImGuiAttachment(renderPassDesc.DepthAttachment);
            ImGui::TreePop();
        }

        changed |= ImGuiEnumCombo("Num Samples", renderPassDesc.NumSamples);

        if (changed)
        {
            m_renderPass = AssetManager::SetAsset(Handle<IRenderPass>::Create(renderPassDesc), m_key);
            AssetManager::SaveAsset(m_renderPass, m_key);
        }
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

bool RenderPassEditorLayer::ImGuiAttachment(AttachmentDesc& attachment)
{
    bool changed = false;
    changed |= ImGuiEnumCombo("Format", attachment.Format);
    changed |= ImGuiEnumCombo("Usage", attachment.Usage);
    changed |= ImGuiEnumCombo("Load Action", attachment.LoadAction);
    changed |= ImGuiEnumCombo("Store Action", attachment.StoreAction);
    return changed;
}

} // namespace Yogi
