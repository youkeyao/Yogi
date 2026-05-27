#include "Renderer/MaterialSchema.h"
#include "Renderer/Material.h"

#include <cstring>

namespace Yogi
{

static uint32_t SizeOfFieldType(MaterialSchema::FieldType t)
{
    switch (t)
    {
        case MaterialSchema::FieldType::Float:
            return 4;
        case MaterialSchema::FieldType::Vec2:
            return 8;
        case MaterialSchema::FieldType::Vec3:
            return 12;
        case MaterialSchema::FieldType::Vec4:
            return 16;
        case MaterialSchema::FieldType::Int:
            return 4;
        case MaterialSchema::FieldType::UInt:
            return 4;
        case MaterialSchema::FieldType::Texture:
            return 4; // bindless slot index, uint
    }
    return 0;
}

void MaterialSchema::AddField(const std::string& name, uint32_t offset, FieldType type, const DefaultValue& def)
{
    Field f{ offset, SizeOfFieldType(type), type };
    m_fields[name] = f;

    if (m_defaults.size() < offset + f.Size)
        m_defaults.resize(offset + f.Size, 0);

    // Stash the default at its byte offset. Variant alternative size must
    // match the field's declared size; a mismatch leaves zero-fill in place
    // (visible at runtime as "default-default", easy to spot).
    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if (sizeof(T) == f.Size)
                std::memcpy(m_defaults.data() + offset, &v, sizeof(T));
        },
        def);
}

void MaterialSchema::Build(uint32_t stride)
{
    m_stride = stride;
    if (m_defaults.size() < stride)
        m_defaults.resize(stride, 0);
}

void MaterialSchema::Pack(const Material& material, uint8_t* outBytes, const TextureResolver& resolver) const
{
    if (m_stride > 0)
        std::memcpy(outBytes, m_defaults.data(), m_stride);

    for (const auto& [name, value] : material.Params)
    {
        auto it = m_fields.find(name);
        if (it == m_fields.end())
            continue;

        const Field& f = it->second;

        if (f.Type == FieldType::Texture)
        {
            // Texture fields are packed as uint indices. Source must be a
            // WRef<ITexture> in Params; uint32_t (raw index) also accepted as
            // a power-user escape hatch. Anything else falls back to default.
            uint32_t idx = 0;
            if (auto* texPtr = std::get_if<WRef<ITexture>>(&value))
            {
                if (resolver)
                    idx = resolver(*texPtr);
            }
            else if (auto* uintPtr = std::get_if<uint32_t>(&value))
            {
                idx = *uintPtr;
            }
            std::memcpy(outBytes + f.Offset, &idx, sizeof(uint32_t));
            continue;
        }

        // Numeric / vector field: visit the variant; the alternative whose
        // sizeof matches the field's declared size wins (others are ignored
        // -- e.g., a Vec3 written into a Float slot is dropped).
        std::visit(
            [&](auto&& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (!std::is_same_v<T, WRef<ITexture>>)
                {
                    if (sizeof(T) == f.Size)
                        std::memcpy(outBytes + f.Offset, &v, sizeof(T));
                }
            },
            value);
    }
}

MaterialSchema& MaterialSchema::Default()
{
    static MaterialSchema s_schema;
    return s_schema;
}

} // namespace Yogi
