#pragma once

#include "Math/Vector.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

class Material;

class YG_API MaterialSchema
{
public:
    enum class FieldType : uint32_t
    {
        Float,
        Vec2,
        Vec3,
        Vec4,
        Int,
        UInt,
        Texture,
    };

    using DefaultValue = std::variant<float, Vector2, Vector3, Vector4, int32_t, uint32_t>;

    using TextureResolver = std::function<uint32_t(const WRef<ITexture>&)>;

    void AddField(const std::string& name, uint32_t offset, FieldType type, const DefaultValue& def);

    void Build(uint32_t stride);

    uint32_t Stride() const { return m_stride; }

    void Pack(const Material& material, uint8_t* outBytes, const TextureResolver& resolver = {}) const;

    static MaterialSchema& Default();

private:
    struct Field
    {
        uint32_t  Offset;
        uint32_t  Size;
        FieldType Type;
    };
    std::unordered_map<std::string, Field> m_fields;
    std::vector<uint8_t>                   m_defaults;
    uint32_t                               m_stride = 0;
};

} // namespace Yogi
