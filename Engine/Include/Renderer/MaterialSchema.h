#pragma once

#include "Math/Vector.h"
#include "Renderer/RHI/ITexture.h"

namespace Yogi
{

class Material;

class YG_API MaterialSchema
{
public:
    enum class FieldKind : uint8_t
    {
        Float,
        Vec2,
        Vec3,
        Vec4,
        Int,
        Uint,
        TextureSlot,
    };

    using DefaultValue = std::variant<float, Vector2, Vector3, Vector4, int32_t, uint32_t>;

    using TextureResolver = std::function<uint32_t(const WRef<ITexture>&)>;

    struct FieldAttribute
    {
        std::string        Name;
        std::vector<float> FloatArgs;
        std::vector<int>   IntArgs;
    };

    struct Field
    {
        std::string                 Name;
        uint32_t                    Offset = 0;
        uint32_t                    Size   = 0;
        FieldKind                   Kind   = FieldKind::Float;
        std::vector<FieldAttribute> Attributes;
    };

    void AddField(const std::string& name, uint32_t offset, FieldKind kind);
    void AddField(const std::string& name, uint32_t offset, FieldKind kind, const DefaultValue& def);

    void AddAttribute(const std::string& fieldName, FieldAttribute attribute);

    void SetDefault(const std::string& name, const DefaultValue& def);

    void Build(uint32_t stride);

    uint32_t Stride() const { return m_stride; }

    const std::vector<Field>& Fields() const { return m_fields; }

    void Pack(const Material& material, uint8_t* outBytes, const TextureResolver& resolver = {}) const;

private:
    std::vector<Field>   m_fields;
    std::vector<uint8_t> m_defaults;
    uint32_t             m_stride = 0;

    Field* FindMutable(const std::string& name);
};

} // namespace Yogi
