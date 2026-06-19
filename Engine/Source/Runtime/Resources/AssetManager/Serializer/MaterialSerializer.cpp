#include "Resources/AssetManager/Serializer/MaterialSerializer.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Renderer/Material.h"
#include "Renderer/MaterialSchema.h"

#include <zpp_bits.h>

namespace Yogi
{

namespace
{
constexpr uint32_t kMatMagic     = 0x4D415459; // 'YTAM'
constexpr uint32_t kMatVersion   = 7;          // v7: MaterialShader is WRef<MaterialSchema>
constexpr uint32_t kMatVersionV6 = 6;
constexpr uint32_t kMatVersionV5 = 5;
constexpr uint32_t kMatVersionV4 = 4;
constexpr uint32_t kMatVersionV3 = 3;
constexpr uint32_t kMatVersionV2 = 2;

// Tag values match the order in MaterialSchema::DefaultValue (which is
// a subset of Material::ParamValue minus the WRef<ITexture> slot).
enum class ParamTag : uint8_t
{
    Float = 0,
    Vec2,
    Vec3,
    Vec4,
    Int32,
    UInt32,
};

// Best-effort tag from a runtime ParamValue. Returns false on the
// WRef<ITexture> slot, which the serializer handles via textureKeys.
bool TagOf(const Material::ParamValue& v, ParamTag& out)
{
    if (std::holds_alternative<float>(v))
    {
        out = ParamTag::Float;
        return true;
    }
    if (std::holds_alternative<Vector2>(v))
    {
        out = ParamTag::Vec2;
        return true;
    }
    if (std::holds_alternative<Vector3>(v))
    {
        out = ParamTag::Vec3;
        return true;
    }
    if (std::holds_alternative<Vector4>(v))
    {
        out = ParamTag::Vec4;
        return true;
    }
    if (std::holds_alternative<int32_t>(v))
    {
        out = ParamTag::Int32;
        return true;
    }
    if (std::holds_alternative<uint32_t>(v))
    {
        out = ParamTag::UInt32;
        return true;
    }
    return false; // texture
}
} // namespace

Owner<Material> MaterialSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    zpp::bits::in inArchive(binary);

    // Peek the first 4 bytes for the v2 magic. If it matches we read the
    // versioned header; otherwise fall back to the v1 format (raw
    // vector<string> textureKeys at the start).
    Owner<Material> material = Owner<Material>::Create();

    uint32_t maybeMagic = 0;
    if (binary.size() >= sizeof(uint32_t))
        std::memcpy(&maybeMagic, binary.data(), sizeof(uint32_t));

    if (maybeMagic != kMatMagic)
    {
        // ---- v1 fallback (Phase 1-3 .mat assets) ----
        std::vector<std::string> textureKeys;
        if (failure(inArchive(textureKeys)))
        {
            YG_CORE_ERROR("Failed to deserialize material '{0}' (v1)", key);
            return nullptr;
        }
        // v1 materials default to Standard schema.
        material->Schema = AssetManager::AcquireAsset<MaterialSchema>(Material::kDefaultMaterialSchemaKey);
        material->Textures.resize(textureKeys.size());
        for (size_t i = 0; i < textureKeys.size(); ++i)
        {
            if (!textureKeys[i].empty())
                material->Textures[i] = AssetManager::AcquireAsset<ITexture>(textureKeys[i]);
        }
        return material;
    }

    // ---- versioned header ----
    uint32_t magic   = 0;
    uint32_t version = 0;
    if (failure(inArchive(magic, version)))
    {
        YG_CORE_ERROR("Failed to deserialize material '{0}' (header)", key);
        return nullptr;
    }

    // v2 carried a materialTypeId field. Read+discard so existing .mat
    // assets keep loading.
    if (version == kMatVersionV2)
    {
        uint16_t typeId = 0;
        if (failure(inArchive(typeId)))
        {
            YG_CORE_ERROR("Failed to deserialize material '{0}' (v2 typeId)", key);
            return nullptr;
        }
        material->Schema = AssetManager::AcquireAsset<MaterialSchema>(Material::kDefaultMaterialSchemaKey);
    }
    else if (version == kMatVersionV3)
    {
        // v3 had no MaterialShader field. Default to Standard.
        material->Schema = AssetManager::AcquireAsset<MaterialSchema>(Material::kDefaultMaterialSchemaKey);
    }
    else if (version >= kMatVersionV4)
    {
        // v4+: MaterialShader is explicitly stored as a string key.
        std::string materialShaderKey;
        if (failure(inArchive(materialShaderKey)))
        {
            YG_CORE_ERROR("Failed to deserialize material '{0}' (v4+ materialShader)", key);
            return nullptr;
        }
        if (materialShaderKey.empty())
            materialShaderKey = std::string{ Material::kDefaultMaterialSchemaKey };

        // v5: skip the embedded schema blob (schema now lives in MaterialSchema asset).
        if (version == kMatVersionV5)
        {
            std::vector<uint8_t> schemaBinary;
            if (failure(inArchive(schemaBinary)))
            {
                YG_CORE_ERROR("Failed to deserialize material '{0}' (v5 schema blob)", key);
                return nullptr;
            }
        }

        material->Schema = AssetManager::AcquireAsset<MaterialSchema>(materialShaderKey);
    }
    else
    {
        YG_CORE_WARN("Material '{0}' has version {1}, expected {2} -- defaulting MaterialShader to Standard "
                     "and attempting forward read",
                     key,
                     version,
                     kMatVersion);
        material->Schema = AssetManager::AcquireAsset<MaterialSchema>(Material::kDefaultMaterialSchemaKey);
    }

    std::vector<std::string> textureKeys;
    if (failure(inArchive(textureKeys)))
    {
        YG_CORE_ERROR("Failed to deserialize material '{0}' (textureKeys)", key);
        return nullptr;
    }

    material->Textures.resize(textureKeys.size());
    for (size_t i = 0; i < textureKeys.size(); ++i)
    {
        if (!textureKeys[i].empty())
            material->Textures[i] = AssetManager::AcquireAsset<ITexture>(textureKeys[i]);
    }

    uint32_t paramCount = 0;
    if (failure(inArchive(paramCount)))
        return material; // tolerate truncated tail

    // Schema comes from MaterialShader WRef.
    const MaterialSchema* schema = material->Schema.Get();

    // Helper: read N raw floats into a Vector view. Returns false on EOF.
    auto readFloats = [&](float* dst, size_t n) -> bool {
        for (size_t k = 0; k < n; ++k)
            if (failure(inArchive(dst[k])))
                return false;
        return true;
    };

    for (uint32_t i = 0; i < paramCount; ++i)
    {
        std::string name;
        uint8_t     tagByte = 0;
        if (failure(inArchive(name, tagByte)))
            return material;

        ParamTag             tag = static_cast<ParamTag>(tagByte);
        Material::ParamValue value;
        switch (tag)
        {
            case ParamTag::Float:
            {
                float v = 0.0f;
                if (failure(inArchive(v)))
                    return material;
                value = v;
                break;
            }
            case ParamTag::Vec2:
            {
                Vector2 v;
                if (!readFloats(&v.x, 2))
                    return material;
                value = v;
                break;
            }
            case ParamTag::Vec3:
            {
                Vector3 v;
                if (!readFloats(&v.x, 3))
                    return material;
                value = v;
                break;
            }
            case ParamTag::Vec4:
            {
                Vector4 v;
                if (!readFloats(&v.x, 4))
                    return material;
                value = v;
                break;
            }
            case ParamTag::Int32:
            {
                int32_t v = 0;
                if (failure(inArchive(v)))
                    return material;
                value = v;
                break;
            }
            case ParamTag::UInt32:
            {
                uint32_t v = 0;
                if (failure(inArchive(v)))
                    return material;
                value = v;
                break;
            }
            default:
                YG_CORE_WARN("Material '{0}' field '{1}' has unknown tag {2}, skipping", key, name, (int)tagByte);
                continue;
        }

        // Drop fields the current schema doesn't know about.
        if (schema)
        {
            bool known = false;
            for (const auto& f : schema->Fields())
            {
                if (f.Name == name)
                {
                    known = true;
                    break;
                }
            }
            if (!known)
                continue;
        }

        material->Params[name] = std::move(value);
    }

    return material;
}

std::vector<uint8_t> MaterialSerializer::Serialize(const WRef<Material>& asset, const std::string& key)
{
    std::vector<uint8_t> data;
    zpp::bits::out       outArchive(data);

    std::vector<std::string> textureKeys;
    textureKeys.reserve(asset->Textures.size());
    for (auto& texture : asset->Textures)
        textureKeys.emplace_back(AssetManager::GetAssetKey<ITexture>(texture));

    // Get the MaterialSchema asset key (or default).
    std::string materialShaderKey;
    if (asset->Schema)
        materialShaderKey = AssetManager::GetAssetKey<MaterialSchema>(asset->Schema);
    if (materialShaderKey.empty())
        materialShaderKey = std::string{ Material::kDefaultMaterialSchemaKey };

    if (failure(outArchive(kMatMagic, kMatVersion, materialShaderKey)))
    {
        YG_CORE_ERROR("Failed to serialize material '{0}' (header)", key);
        return {};
    }

    // v7: Schema is no longer embedded; it lives in the MaterialSchema asset.

    if (failure(outArchive(textureKeys)))
    {
        YG_CORE_ERROR("Failed to serialize material '{0}' (textureKeys)", key);
        return {};
    }

    // Walk the schema for stable ordering.
    const MaterialSchema* schema = asset->Schema.Get();

    std::vector<std::pair<std::string, Material::ParamValue>> ordered;
    if (schema)
    {
        for (const auto& f : schema->Fields())
        {
            auto it = asset->Params.find(f.Name);
            if (it == asset->Params.end())
                continue;
            if (f.Kind == MaterialSchema::FieldKind::TextureSlot && std::holds_alternative<WRef<ITexture>>(it->second))
                continue;
            ordered.emplace_back(f.Name, it->second);
        }
    }
    else
    {
        for (const auto& [name, v] : asset->Params)
        {
            if (std::holds_alternative<WRef<ITexture>>(v))
                continue;
            ordered.emplace_back(name, v);
        }
    }

    if (failure(outArchive(static_cast<uint32_t>(ordered.size()))))
    {
        YG_CORE_ERROR("Failed to serialize material '{0}' (param count)", key);
        return {};
    }

    for (auto& [name, v] : ordered)
    {
        ParamTag tag;
        if (!TagOf(v, tag))
            continue;
        if (failure(outArchive(name, static_cast<uint8_t>(tag))))
            return {};

        std::visit(
            [&](auto&& vv) {
                using T = std::decay_t<decltype(vv)>;
                if constexpr (std::is_same_v<T, float>)
                {
                    (void)outArchive(vv);
                }
                else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>)
                {
                    (void)outArchive(vv);
                }
                else if constexpr (std::is_same_v<T, Vector2>)
                {
                    (void)outArchive(vv.x, vv.y);
                }
                else if constexpr (std::is_same_v<T, Vector3>)
                {
                    (void)outArchive(vv.x, vv.y, vv.z);
                }
                else if constexpr (std::is_same_v<T, Vector4>)
                {
                    (void)outArchive(vv.x, vv.y, vv.z, vv.w);
                }
            },
            v);
    }
    return data;
}

} // namespace Yogi
