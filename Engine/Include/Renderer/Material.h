#pragma once

#include "Math/Vector.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{
class YG_API Material
{
public:
    using ParamValue = std::variant<float, Vector2, Vector3, Vector4, int32_t, uint32_t, WRef<ITexture>>;

    Material()  = default;
    ~Material() = default;

    std::unordered_map<std::string, ParamValue> Params;
    std::vector<WRef<ITexture>>                 Textures;
};

} // namespace Yogi
