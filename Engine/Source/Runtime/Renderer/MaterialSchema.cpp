#include "Renderer/MaterialSchema.h"
#include "Renderer/Material.h"

#include <cstring>

namespace Yogi
{

static uint32_t SizeOfFieldKind(MaterialSchema::FieldKind k)
{
    switch (k)
    {
        case MaterialSchema::FieldKind::Float:
            return 4;
        case MaterialSchema::FieldKind::Vec2:
            return 8;
        case MaterialSchema::FieldKind::Vec3:
            return 12;
        case MaterialSchema::FieldKind::Vec4:
            return 16;
        case MaterialSchema::FieldKind::Int:
            return 4;
        case MaterialSchema::FieldKind::Uint:
            return 4;
        case MaterialSchema::FieldKind::TextureSlot:
            return 4;
    }
    return 0;
}

MaterialSchema::Field* MaterialSchema::FindMutable(const std::string& name)
{
    for (auto& f : m_fields)
    {
        if (f.Name == name)
            return &f;
    }
    return nullptr;
}

void MaterialSchema::AddField(const std::string& name, uint32_t offset, FieldKind kind)
{
    Field f;
    f.Name   = name;
    f.Offset = offset;
    f.Size   = SizeOfFieldKind(kind);
    f.Kind   = kind;
    m_fields.push_back(std::move(f));

    if (m_defaults.size() < offset + SizeOfFieldKind(kind))
        m_defaults.resize(offset + SizeOfFieldKind(kind), 0);
}

void MaterialSchema::AddField(const std::string& name, uint32_t offset, FieldKind kind, const DefaultValue& def)
{
    AddField(name, offset, kind);

    Field* f = FindMutable(name);
    if (!f)
        return;

    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if (sizeof(T) == f->Size)
                std::memcpy(m_defaults.data() + f->Offset, &v, sizeof(T));
        },
        def);
}

void MaterialSchema::AddAttribute(const std::string& fieldName, FieldAttribute attribute)
{
    if (Field* f = FindMutable(fieldName))
        f->Attributes.push_back(std::move(attribute));
}

void MaterialSchema::SetDefault(const std::string& name, const DefaultValue& def)
{
    Field* f = FindMutable(name);
    if (!f)
        return;

    if (m_defaults.size() < f->Offset + f->Size)
        m_defaults.resize(f->Offset + f->Size, 0);

    std::visit(
        [&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if (sizeof(T) == f->Size)
                std::memcpy(m_defaults.data() + f->Offset, &v, sizeof(T));
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
        const Field* f = nullptr;
        for (const auto& candidate : m_fields)
        {
            if (candidate.Name == name)
            {
                f = &candidate;
                break;
            }
        }
        if (!f)
            continue;

        if (f->Kind == FieldKind::TextureSlot)
        {
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
            std::memcpy(outBytes + f->Offset, &idx, sizeof(uint32_t));
            continue;
        }

        std::visit(
            [&](auto&& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (!std::is_same_v<T, WRef<ITexture>>)
                {
                    if (sizeof(T) == f->Size)
                        std::memcpy(outBytes + f->Offset, &v, sizeof(T));
                }
            },
            value);
    }
}

} // namespace Yogi
