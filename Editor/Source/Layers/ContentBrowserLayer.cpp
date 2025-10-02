#include "Layers/ContentBrowserLayer.h"

#include "Utils/fontawesome4_header.h"

#include <imgui.h>

namespace Yogi
{

// Project Directory
ContentBrowserLayer::ContentBrowserLayer() :
    Layer("Content Browser Layer"),
    m_baseDirectory("."),
    m_relativeDirectory(".")
{}

void ContentBrowserLayer::OnUpdate(Timestep ts)
{
    ImGui::Begin("Content Browser");
    float        panelWidth = ImGui::GetContentRegionAvail().x;
    static float padding    = 8.0f;

    // new content
    if (ImGui::BeginPopupContextWindow())
    {
        if (ImGui::MenuItem("Create Material"))
        {
            Ref<IPipeline>   pipeline = AssetManager::GetAsset<IPipeline>("EngineAssets/Pipelines/Default.pipeline");
            Handle<Material> material = Handle<Material>::Create();
            material->SetPipeline(pipeline);
            std::string name = "NewMaterial";
            while (std::filesystem::exists(m_baseDirectory / m_relativeDirectory / (name + ".mat")))
            {
                name = "_" + name;
            }
            AssetManager::SaveAsset(Ref<Material>::Create(material),
                                    (m_relativeDirectory / (name + ".mat")).lexically_normal().generic_string());
        }
        if (ImGui::MenuItem("Create Render Pass"))
        {
            auto&               swapChain  = Application::GetInstance().GetSwapChain();
            Handle<IRenderPass> renderPass = Handle<IRenderPass>::Create(RenderPassDesc{
                { AttachmentDesc{ swapChain->GetColorFormat(), AttachmentUsage::Present } },
                AttachmentDesc{
                    swapChain->GetDepthFormat(), AttachmentUsage::DepthStencil, LoadOp::Clear, StoreOp::DontCare },
                swapChain->GetNumSamples() });
            std::string         name       = "NewRenderPass";
            while (std::filesystem::exists(m_baseDirectory / m_relativeDirectory / (name + ".rp")))
            {
                name = "_" + name;
            }
            AssetManager::SaveAsset(Ref<IRenderPass>::Create(renderPass),
                                    (m_relativeDirectory / (name + ".rp")).lexically_normal().generic_string());
        }
        if (ImGui::MenuItem("Create Pipeline"))
        {
            auto shaderResourceBinding = Yogi::ResourceManager::GetResource<Yogi::IShaderResourceBinding>(
                std::vector<Yogi::ShaderResourceAttribute>{ Yogi::ShaderResourceAttribute{
                    0, 1, Yogi::ShaderResourceType::Buffer, Yogi::ShaderStage::Vertex } });
            Handle<IPipeline> pipeline = Handle<IPipeline>::Create(
                PipelineDesc{ { AssetManager::GetAsset<ShaderDesc>("EngineAssets/Shaders/Test.vert"),
                                AssetManager::GetAsset<ShaderDesc>("EngineAssets/Shaders/Test.frag") },
                              { Yogi::VertexAttribute{ "a_Position", 0, 12, Yogi::ShaderElementType::Float3 },
                                Yogi::VertexAttribute{ "a_TexCoord", 12, 8, Yogi::ShaderElementType::Float2 } },
                              shaderResourceBinding,
                              AssetManager::GetAsset<IRenderPass>("EngineAssets/RenderPasses/Default.rp"),
                              0,
                              PrimitiveTopology::TriangleList });
            std::string name = "NewPipeline";
            while (std::filesystem::exists(m_baseDirectory / m_relativeDirectory / (name + ".pipeline")))
            {
                name = "_" + name;
            }
            AssetManager::SaveAsset(Ref<IPipeline>::Create(pipeline),
                                    (m_relativeDirectory / (name + ".pipeline")).lexically_normal().generic_string());
        }
        ImGui::EndPopup();
    }

    // path
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::Button(".");

    float nowX = ImGui::GetItemRectSize().x + padding;
    if (ImGui::IsItemClicked())
    {
        m_relativeDirectory = ".";
    }
    std::list<std::filesystem::path> paths;
    for (auto p = m_relativeDirectory; p != "."; p = p.parent_path())
    {
        paths.emplace_front(p);
    }

    for (auto& p : paths)
    {
        float textSize = ImGui::CalcTextSize("/").x;
        if (nowX + textSize < panelWidth)
            ImGui::SameLine();
        else
            nowX = padding;
        ImGui::Text("/");
        nowX += ImGui::GetItemRectSize().x + padding;
        textSize = ImGui::CalcTextSize(p.filename().string().c_str()).x;
        if (nowX + textSize < panelWidth)
            ImGui::SameLine();
        else
            nowX = padding;
        ImGui::Button(p.filename().string().c_str());
        nowX += ImGui::GetItemRectSize().x + padding;
        if (ImGui::IsItemClicked())
        {
            m_relativeDirectory = p;
        }
    }
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);

    // content
    static float thumbnailSize = 80.0f;
    float        cellSize      = thumbnailSize + padding;
    int          columnCount   = (int)(panelWidth / cellSize);
    if (columnCount < 1)
        columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    for (auto& directoryEntry : std::filesystem::directory_iterator(m_baseDirectory / m_relativeDirectory))
    {
        const auto& path     = directoryEntry.path();
        std::string filename = path.filename().string();

        const char* icon = directoryEntry.is_directory() ? ICON_FA_FOLDER : ICON_FA_FILE;

        ImGui::Selectable(("##" + path.string()).c_str(), false, 0, { thumbnailSize, thumbnailSize });
        auto oldPos   = ImGui::GetCursorPos();
        auto iconSize = ImGui::CalcTextSize(icon);

        if (ImGui::BeginDragDropSource())
        {
            auto itemPath = path.lexically_normal().generic_string();
            ImGui::SetDragDropPayload("ContentBrowserItem", itemPath.data(), itemPath.size() + 1);
            ImGui::EndDragDropSource();
        }
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (ImGui::IsItemHovered() && directoryEntry.is_directory())
            {
                m_relativeDirectory /= path.filename();
            }
        }

        ImGui::SetCursorPosX(oldPos.x + thumbnailSize / 2 - iconSize.x / 2);
        ImGui::SetCursorPosY(oldPos.y - cellSize / 2 + iconSize.y / 2);
        ImGui::Text("%s", icon);
        auto textSize = ImGui::CalcTextSize(filename.c_str());
        ImGui::SetCursorPosY(oldPos.y);
        if (textSize.x >= cellSize)
        {
            ImGui::SetCursorPosY(oldPos.y);
            ImGui::TextWrapped("%s", filename.c_str());
        }
        else
        {
            ImGui::SetCursorPosX(oldPos.x + thumbnailSize / 2 - textSize.x / 2);
            ImGui::Text("%s", filename.c_str());
        }

        ImGui::NextColumn();
    }

    ImGui::End();
}

void ContentBrowserLayer::OnEvent(Event& event) {}

} // namespace Yogi
