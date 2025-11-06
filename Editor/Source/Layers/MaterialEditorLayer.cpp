#include "Layers/MaterialEditorLayer.h"
#include "Registry/AssetRegistry.h"
#include "Utils/ImGuiEnumCombo.h"
#include "Resources/ResourceManager/ResourceManager.h"

#include <imgui.h>

namespace Yogi
{

MaterialEditorLayer::MaterialEditorLayer() : Layer("Material Editor Layer") {}

void MaterialEditorLayer::OnUpdate(Timestep ts)
{
    ImGui::Begin("Material Editor");
    float cursorY = ImGui::GetCursorPosY();

    if (m_material)
    {
        ImGui::TextUnformatted(m_key.c_str());
        ImGui::Separator();
        bool changed = false;

        ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_FramePadding;
        int                removeIndex   = -1;
        for (int i = 0; i < m_material->GetPasses().size(); ++i)
        {
            auto pass     = m_material->GetPasses()[i];
            bool isOpened = ImGui::TreeNodeEx(("Pass" + std::to_string(i)).c_str(), treeNodeFlags);
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Pass"))
                {
                    removeIndex = i;
                }
                ImGui::EndPopup();
            }
            if (isOpened)
            {
                changed |= ImGuiPipeline(pass.Pipeline);
                auto&    vertexLayout = pass.Pipeline->GetDesc().VertexLayout;
                auto&    data         = pass.Data;
                auto&    textures     = pass.Textures;
                uint32_t textureIndex = 0;
                for (auto& element : vertexLayout)
                {
                    if (element.Usage == AttributeUsage::TextureID)
                    {
                        auto&       texture    = textures[textureIndex++].second;
                        std::string textureKey = texture ? AssetManager::GetAssetKey(texture) : "None";
                        if (ImGui::BeginCombo(("##" + textureKey).c_str(), textureKey.c_str()))
                        {
                            for (auto& key : AssetRegistry::GetKeys<ITexture>())
                            {
                                if (ImGui::Selectable(key.c_str(), key == textureKey))
                                {
                                    texture = AssetManager::GetAsset<ITexture>(key);
                                    changed |= true;
                                }
                            }
                            ImGui::EndCombo();
                        }
                    }
                    else if (element.Usage == AttributeUsage::Custom)
                    {
                        if (element.Type == ShaderElementType::Float)
                        {
                            changed |= ImGui::InputFloat(element.Name.c_str(), (float*)(data.data() + element.Offset));
                        }
                        else if (element.Type == ShaderElementType::Float2)
                        {
                            changed |= ImGui::InputFloat2(element.Name.c_str(), (float*)(data.data() + element.Offset));
                        }
                        else if (element.Type == ShaderElementType::Float3)
                        {
                            changed |= ImGui::InputFloat3(element.Name.c_str(), (float*)(data.data() + element.Offset));
                        }
                        else if (element.Type == ShaderElementType::Float4)
                        {
                            changed |= ImGui::ColorEdit4(element.Name.c_str(), (float*)(data.data() + element.Offset));
                        }
                    }
                }
                m_material->SetPass(i, pass);
                ImGui::TreePop();
            }
        }
        if (removeIndex >= 0)
        {
            m_material->RemovePass(removeIndex);
            changed |= true;
        }

        if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
        {
            auto shaderResourceBinding = Yogi::ResourceManager::GetResource<Yogi::IShaderResourceBinding>(
                std::vector<Yogi::ShaderResourceAttribute>{ Yogi::ShaderResourceAttribute{
                    0, 1, Yogi::ShaderResourceType::Buffer, Yogi::ShaderStage::Vertex } });
            m_material->AddPass(ResourceManager::GetResource<IPipeline>(
                PipelineDesc{ { AssetManager::GetAsset<ShaderDesc>("EngineAssets/Shaders/Test.vert"),
                                AssetManager::GetAsset<ShaderDesc>("EngineAssets/Shaders/Test.frag") },
                              { Yogi::VertexAttribute{ "a_Position", 0, 12, Yogi::ShaderElementType::Float3 },
                                Yogi::VertexAttribute{ "a_TexCoord", 12, 8, Yogi::ShaderElementType::Float2 } },
                              shaderResourceBinding,
                              AssetManager::GetAsset<IRenderPass>("EngineAssets/RenderPasses/Default.rp"),
                              0,
                              PrimitiveTopology::TriangleList }));
            changed |= true;
        }

        if (changed)
        {
            AssetManager::SaveAsset(m_material, m_key);
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
            if (fpath.extension().string() == ".mat")
            {
                m_key      = path;
                m_material = AssetManager::GetAsset<Material>(m_key);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void MaterialEditorLayer::OnEvent(Event& event) {}

// -----------------------------------------------------------------------------------
bool MaterialEditorLayer::ImGuiPipeline(Ref<IPipeline>& pipeline)
{
    bool               changed       = false;
    PipelineDesc       pipelineDesc  = pipeline->GetDesc();
    auto               removeIt      = pipelineDesc.Shaders.end();
    ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_FramePadding;
    if (ImGui::TreeNodeEx("Pipeline", treeNodeFlags))
    {
        if (ImGui::TreeNodeEx("Shaders", treeNodeFlags))
        {
            std::vector<Ref<ShaderDesc>>::iterator removeIt = pipelineDesc.Shaders.end();
            for (auto it = pipelineDesc.Shaders.begin(); it != pipelineDesc.Shaders.end(); ++it)
            {
                auto&       shader    = *it;
                std::string shaderKey = shader ? AssetManager::GetAssetKey(shader) : "None";
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::BeginCombo(("##" + shaderKey).c_str(), shaderKey.c_str()))
                {
                    for (auto& key : AssetRegistry::GetKeys<ShaderDesc>())
                    {
                        if (ImGui::Selectable(key.c_str(), key == shaderKey))
                        {
                            shader = AssetManager::GetAsset<ShaderDesc>(key);
                            changed |= true;
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Delete Shader"))
                    {
                        removeIt = it;
                        changed |= true;
                    }
                    ImGui::EndPopup();
                }
            }
            if (removeIt != pipelineDesc.Shaders.end())
            {
                pipelineDesc.Shaders.erase(removeIt);
            }
            if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
            {
                pipelineDesc.Shaders.push_back(nullptr);
                changed |= true;
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Attribute Usage", treeNodeFlags))
        {
            for (auto& element : pipelineDesc.VertexLayout)
            {
                changed |= ImGuiEnumCombo(element.Name.c_str(), element.Usage);
            }
            ImGui::TreePop();
        }

        std::string renderPassKey = AssetManager::GetAssetKey(pipelineDesc.RenderPass);
        if (ImGui::BeginCombo("Render Pass", renderPassKey.c_str()))
        {
            for (auto& key : AssetRegistry::GetKeys<IRenderPass>())
            {
                if (ImGui::Selectable(key.c_str(), key == renderPassKey))
                {
                    pipelineDesc.RenderPass = AssetManager::GetAsset<IRenderPass>(key);
                    changed |= true;
                }
            }
            ImGui::EndCombo();
        }

        changed |= ImGui::InputInt("SubPass Index", &pipelineDesc.SubPassIndex);
        changed |= ImGuiEnumCombo("Topology", pipelineDesc.Topology);

        ImGui::TreePop();
    }

    if (changed)
    {
        if (ValidatePipelineDesc(pipelineDesc))
        {
            pipeline = ResourceManager::GetResource<IPipeline>(pipelineDesc);
        }
    }

    return changed;
}

bool MaterialEditorLayer::ValidatePipelineDesc(PipelineDesc& desc)
{
    int flag = 0;
    for (auto& shader : desc.Shaders)
    {
        if (shader)
        {
            if (flag & (1 << (int)shader->Stage))
            {
                return false;
            }
            flag |= (1 << (int)shader->Stage);
        }
    }
    return true;
}

} // namespace Yogi
