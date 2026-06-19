#include "Resources/AssetManager/Serializer/MaterialSchemaSerializer.h"
#include "Resources/AssetManager/Serializer/SlangShaderCompiler.h"

namespace Yogi
{

// Derive "StandardMaterial" from key "EngineAssets/Shaders/Materials/Standard.slang"
static std::string KeyToMaterialTypeName(const std::string& key)
{
    std::filesystem::path p(key);
    std::string           stem = p.stem().string(); // "Standard"
    return stem + "Material";
}

Owner<MaterialSchema> MaterialSchemaSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    if (binary.empty())
        return nullptr;

    std::string           slangSource(binary.begin(), binary.end());
    std::string           materialTypeName = KeyToMaterialTypeName(key);
    std::filesystem::path sourcePath(key);

    MaterialSchema schema = SlangShaderCompiler::ReflectMaterialSchema(slangSource, sourcePath, materialTypeName);

    if (schema.Stride() == 0)
    {
        YG_CORE_WARN(
            "MaterialSchemaSerializer: failed to reflect schema for '{0}' (type='{1}')", key, materialTypeName);
        return nullptr;
    }

    return Owner<MaterialSchema>::Create(std::move(schema));
}

std::vector<uint8_t> MaterialSchemaSerializer::Serialize(const WRef<MaterialSchema>& asset, const std::string& key)
{
    // MaterialSchema is derived from source; we don't serialize the schema
    // back to disk.  Return empty so SaveAsset() becomes a no-op.
    return {};
}

} // namespace Yogi
