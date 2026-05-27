#pragma once

#include "Math/Vector.h"
#include "Renderer/RHI/ITexture.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Yogi
{

class Material;

// Engine-wide Material parameter schema. One global instance describes the
// layout of the GPU-side MaterialData struct: which named parameters exist,
// where each lives, and what default value it gets when a Material doesn't
// override it.
//
// Material itself stores a name-keyed Params dictionary and never references
// a shader / pipeline. At upload time, the renderer asks the schema to Pack()
// a Material into a byte buffer matching the GPU layout. Unknown param names
// are ignored; missing names fall back to the registered default.
//
// Adding a new field is a two-step:
//   1. Edit struct MaterialData in ShaderData.h (C++ + GLSL share it)
//   2. Add one MaterialSchema::Default().AddField(...) call at engine init
// Shaders see the new field automatically; existing Materials get its default.
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
        // Bindless texture handle. Stored on the Material as WRef<ITexture>;
        // packed into the GPU MaterialData as a uint index resolved through
        // the Pack() callback (typically FRS::RegisterTexture).
        Texture,
    };

    using DefaultValue = std::variant<float, Vector2, Vector3, Vector4, int32_t, uint32_t>;

    // Map a Texture-typed Material::Params value (WRef<ITexture>) to the
    // bindless u_textures[] slot index. Pack() invokes this when packing a
    // Texture-typed field; if the resolver is empty or the param isn't a
    // texture, the field's default uint stands in (typically 0 = default
    // white). The renderer supplies the resolver each frame.
    using TextureResolver = std::function<uint32_t(const WRef<ITexture>&)>;

    // Register a field. `offset` should match offsetof(MaterialData, name).
    // The default value's type must agree with `type`, otherwise the default
    // is not applied (zero-fill takes over for that range).
    void AddField(const std::string& name, uint32_t offset, FieldType type, const DefaultValue& def);

    // Snapshot the schema. Call once after all AddField() calls -- pads the
    // defaults blob to `stride` bytes (matches sizeof(MaterialData) GPU-side).
    void Build(uint32_t stride);

    uint32_t Stride() const { return m_stride; }

    // Pack a Material into outBytes (must be at least Stride() bytes). Starts
    // from the defaults blob, then memcpy each Material::Params override into
    // its declared offset.
    void Pack(const Material& material, uint8_t* outBytes, const TextureResolver& resolver = {}) const;

    // Engine-wide singleton schema. Registered at FRS startup. Editor and
    // serializer query this to enumerate fields.
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
