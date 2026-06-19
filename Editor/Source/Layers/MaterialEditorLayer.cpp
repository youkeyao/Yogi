#include "Layers/MaterialEditorLayer.h"
#include "Registry/AssetRegistry.h"
#include "Utils/ImGuiEnumCombo.h"
#include "Resources/ResourceManager/ResourceManager.h"

#include <imgui.h>

namespace Yogi
{

MaterialEditorLayer::MaterialEditorLayer() : Layer("Material Editor Layer") {}

// ---- helpers ------------------------------------------------------------

namespace
{
// Editor-only view of the field's authoring intent. We compute this
// once per field per frame from the engine-side raw attribute list;
// the engine itself stays free of "Slider" / "Color" semantics.
enum class EditorHint
{
    Default,
    Slider,
    Color,
    HDR,
    Texture,
};

struct EditorView
{
    EditorHint hint      = EditorHint::Default;
    float      sliderMin = 0.0f;
    float      sliderMax = 1.0f;
};

EditorView ResolveEditorView(const MaterialSchema::Field& f)
{
    EditorView v;

    // Texture/Color/HDR are flags; Range carries [min, max]. When more
    // than one attribute is present, the most-specific wins (Range >
    // HDR > Color > Texture > Default), matching the priority a user
    // would expect when authoring.
    for (const auto& a : f.Attributes)
    {
        if (a.Name == "Range" && a.FloatArgs.size() >= 2)
        {
            v.hint      = EditorHint::Slider;
            v.sliderMin = a.FloatArgs[0];
            v.sliderMax = a.FloatArgs[1];
        }
        else if (a.Name == "HDR")
        {
            if (v.hint == EditorHint::Default || v.hint == EditorHint::Color || v.hint == EditorHint::Texture)
                v.hint = EditorHint::HDR;
        }
        else if (a.Name == "Color")
        {
            if (v.hint == EditorHint::Default || v.hint == EditorHint::Texture)
                v.hint = EditorHint::Color;
        }
        else if (a.Name == "Texture")
        {
            if (v.hint == EditorHint::Default)
                v.hint = EditorHint::Texture;
        }
    }

    // Even without an explicit [Texture] attribute, a TextureSlot field
    // must surface as a drop target -- the slot is conceptually a
    // texture handle, not a raw uint.
    if (f.Kind == MaterialSchema::FieldKind::TextureSlot && v.hint == EditorHint::Default)
        v.hint = EditorHint::Texture;

    return v;
}

template <typename T>
T GetParam(const Material& mat, const MaterialSchema::Field& f)
{
    auto it = mat.Params.find(f.Name);
    if (it != mat.Params.end())
    {
        if (auto* p = std::get_if<T>(&it->second))
            return *p;
    }
    // No param yet -- editor surfaces zero. The engine's slab row uses
    // the schema's defaults blob, so the rendered value can be non-zero
    // even when the editor shows zero; saving once writes the editor's
    // value into Params and resolves the discrepancy.
    return T{};
}

bool RenderField(Material& mat, const MaterialSchema::Field& f)
{
    bool       changed = false;
    EditorView view    = ResolveEditorView(f);

    ImGui::PushID(f.Name.c_str());
    ImGui::TextUnformatted(f.Name.c_str());
    ImGui::SameLine();

    switch (f.Kind)
    {
        case MaterialSchema::FieldKind::Float:
        {
            float v      = GetParam<float>(mat, f);
            bool  edited = (view.hint == EditorHint::Slider) ?
                ImGui::SliderFloat("##v", &v, view.sliderMin, view.sliderMax) :
                ImGui::InputFloat("##v", &v);
            if (edited)
            {
                mat.Params[f.Name] = v;
                changed            = true;
            }
            break;
        }
        case MaterialSchema::FieldKind::Vec2:
        {
            Vector2 v = GetParam<Vector2>(mat, f);
            if (ImGui::InputFloat2("##v", &v.x))
            {
                mat.Params[f.Name] = v;
                changed            = true;
            }
            break;
        }
        case MaterialSchema::FieldKind::Vec3:
        {
            Vector3 v      = GetParam<Vector3>(mat, f);
            bool    edited = false;
            switch (view.hint)
            {
                case EditorHint::Color:
                    edited = ImGui::ColorEdit3("##v", &v.x);
                    break;
                case EditorHint::HDR:
                    edited = ImGui::ColorEdit3("##v", &v.x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
                    break;
                case EditorHint::Slider:
                    edited = ImGui::SliderFloat3("##v", &v.x, view.sliderMin, view.sliderMax);
                    break;
                default:
                    edited = ImGui::InputFloat3("##v", &v.x);
                    break;
            }
            if (edited)
            {
                mat.Params[f.Name] = v;
                changed            = true;
            }
            break;
        }
        case MaterialSchema::FieldKind::Vec4:
        {
            Vector4 v      = GetParam<Vector4>(mat, f);
            bool    edited = false;
            switch (view.hint)
            {
                case EditorHint::Color:
                    edited = ImGui::ColorEdit4("##v", &v.x);
                    break;
                case EditorHint::HDR:
                    edited = ImGui::ColorEdit4("##v", &v.x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
                    break;
                default:
                    edited = ImGui::InputFloat4("##v", &v.x);
                    break;
            }
            if (edited)
            {
                mat.Params[f.Name] = v;
                changed            = true;
            }
            break;
        }
        case MaterialSchema::FieldKind::Int:
        {
            int v = GetParam<int32_t>(mat, f);
            if (ImGui::InputInt("##v", &v))
            {
                mat.Params[f.Name] = static_cast<int32_t>(v);
                changed            = true;
            }
            break;
        }
        case MaterialSchema::FieldKind::Uint:
        {
            int v = static_cast<int>(GetParam<uint32_t>(mat, f));
            if (ImGui::InputInt("##v", &v))
            {
                if (v < 0)
                    v = 0;
                mat.Params[f.Name] = static_cast<uint32_t>(v);
                changed            = true;
            }
            break;
        }
        case MaterialSchema::FieldKind::TextureSlot:
        {
            ImGui::Button("[texture]");
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ContentBrowserItem"))
                {
                    const char*           path  = (const char*)payload->Data;
                    std::filesystem::path fpath = path;
                    std::string           ext   = fpath.extension().string();
                    if (ext == ".png" || ext == ".jpg" || ext == ".rt")
                    {
                        auto tex = AssetManager::AcquireAsset<ITexture>(path);
                        if (tex)
                        {
                            mat.Params[f.Name] = tex;
                            changed            = true;
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }
            break;
        }
    }

    ImGui::PopID();
    return changed;
}
} // namespace

void MaterialEditorLayer::OnUpdate(Timestep ts)
{
    ImGui::Begin("Material Editor");
    float cursorY = ImGui::GetCursorPosY();

    if (m_material)
    {
        ImGui::TextUnformatted(m_key.c_str());
        ImGui::Text("Schema: %s", AssetManager::GetAssetKey<MaterialSchema>(m_material->Schema).c_str());
        ImGui::Separator();

        WRef<MaterialSchema> ms = m_material->Schema;
        if (ms && ms->Stride() > 0)
        {
            const MaterialSchema& schema = *ms;
            for (const auto& f : schema.Fields())
                RenderField(*m_material, f);

            ImGui::Separator();
            if (ImGui::Button("Save"))
            {
                AssetManager::SaveAsset<Material>(m_material, m_key);
            }
        }
        else
        {
            ImGui::TextColored(ImVec4{ 1, 0.4f, 0.4f, 1 }, "No schema registered for material");
        }

        ImGui::Separator();
        ImGui::Text("Textures: %d", static_cast<int>(m_material->Textures.size()));
    }

    // blank space + drag-drop target for swapping the editor's open .mat.
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
                m_material = AssetManager::AcquireAsset<Material>(m_key);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void MaterialEditorLayer::OnEvent(Event& event) {}

// -----------------------------------------------------------------------------------
bool MaterialEditorLayer::ImGuiMaterialData(const std::vector<std::string> shaderKeys, std::vector<uint8_t>& data)
{
    bool changed = false;

    return changed;
}

} // namespace Yogi
