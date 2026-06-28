#pragma once

#include "Math/Vector.h"
#include "Renderer/RHI/ITexture.h"
#include "Renderer/MaterialSchema.h"

namespace Yogi
{

struct Material
{
    using ParamValue = std::variant<float, Vector2, Vector3, Vector4, int32_t, uint32_t, WRef<ITexture>>;

    static constexpr const char* kDefaultMaterialSchemaKey = "EngineAssets/Shaders/Materials/Standard.slang";

    WRef<MaterialSchema>                        Schema{ nullptr };
    std::unordered_map<std::string, ParamValue> Params;
    std::vector<WRef<ITexture>>                 Textures;
};

} // namespace Yogi
