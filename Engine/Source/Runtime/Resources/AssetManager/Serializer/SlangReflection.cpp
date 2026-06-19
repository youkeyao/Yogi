#include "Resources/AssetManager/Serializer/SlangReflection.h"

#include <slang.h>

namespace Yogi
{

// ---- helpers ------------------------------------------------------------

static MaterialSchema::FieldKind ScalarKindToFieldKind(slang::TypeReflection::ScalarType s)
{
    using S = slang::TypeReflection::ScalarType;
    switch (s)
    {
        case S::Float32:
            return MaterialSchema::FieldKind::Float;
        case S::Int32:
            return MaterialSchema::FieldKind::Int;
        case S::UInt32:
            return MaterialSchema::FieldKind::Uint;
        default:
            return MaterialSchema::FieldKind::Float;
    }
}

static bool ResolveFieldKind(slang::TypeLayoutReflection* fieldLayout, MaterialSchema::FieldKind& outKind)
{
    if (!fieldLayout)
        return false;

    slang::TypeReflection* type = fieldLayout->getType();
    if (!type)
        return false;

    using K = slang::TypeReflection::Kind;
    switch (type->getKind())
    {
        case K::Scalar:
            outKind = ScalarKindToFieldKind(type->getScalarType());
            return true;

        case K::Vector:
        {
            auto scalar = type->getScalarType();
            if (scalar != slang::TypeReflection::ScalarType::Float32)
                return false; // integer/half vectors not yet wired
            switch (type->getElementCount())
            {
                case 2:
                    outKind = MaterialSchema::FieldKind::Vec2;
                    return true;
                case 3:
                    outKind = MaterialSchema::FieldKind::Vec3;
                    return true;
                case 4:
                    outKind = MaterialSchema::FieldKind::Vec4;
                    return true;
                default:
                    return false;
            }
        }

        default:
            return false;
    }
}

struct AttrSpec
{
    const char* slangName;   // suffix-stripped name as in `findUserAttributeByName`
    const char* publicName;  // name surfaced on MaterialSchema::FieldAttribute
    bool        isFloatArgs; // true -> getArgumentValueFloat, else int
};

static const AttrSpec kAttrSpecs[] = {
    // Editor hints -- engine doesn't interpret, just transports.
    { "Range", "Range", true },
    { "Color", "Color", true }, // reads 4 floats (r,g,b,a) as default value
    { "HDR", "HDR", true },
    { "Texture", "Texture", true },
};

struct DecodedDefault
{
    bool                                                                              present = false;
    std::variant<std::monostate, float, Vector2, Vector3, Vector4, int32_t, uint32_t> value;
};

static void CollectFieldAttributes(slang::IGlobalSession*                       globalSession,
                                   slang::VariableReflection*                   var,
                                   std::vector<MaterialSchema::FieldAttribute>& outAttrs,
                                   bool&                                        outIsTexture,
                                   DecodedDefault&                              outDefault)
{
    if (!var)
        return;

    for (const AttrSpec& spec : kAttrSpecs)
    {
        slang::UserAttribute* attr = var->findUserAttributeByName(globalSession, spec.slangName);
        if (!attr)
            continue;

        MaterialSchema::FieldAttribute fa;
        fa.Name        = spec.publicName;
        unsigned int n = attr->getArgumentCount();
        fa.FloatArgs.reserve(n);
        for (unsigned int i = 0; i < n; ++i)
        {
            float v = 0.0f;
            if (SLANG_SUCCEEDED(attr->getArgumentValueFloat(i, &v)))
                fa.FloatArgs.push_back(v);
        }
        outAttrs.push_back(std::move(fa));

        if (std::string_view(spec.slangName) == "Texture")
            outIsTexture = true;

        // [Color(r,g,b,a)]: if the attribute carries 4 float arguments,
        // also treat them as the default value for the field (Vec4 kind).
        // Use fa.FloatArgs.size() instead of n: getArgumentValueFloat() may
        // fail for some indices, leaving fa.FloatArgs smaller than n.
        if (std::string_view(spec.slangName) == "Color" && fa.FloatArgs.size() >= 4)
        {
            float r            = fa.FloatArgs[0];
            float g            = fa.FloatArgs[1];
            float b            = fa.FloatArgs[2];
            float a            = fa.FloatArgs[3];
            outDefault.present = true;
            outDefault.value   = Vector4{ r, g, b, a };
        }

        // [Range(min, max, default)]: if the attribute carries 3+ float
        // arguments, treat the third as the default value for the field.
        if (std::string_view(spec.slangName) == "Range" && fa.FloatArgs.size() >= 3)
        {
            outDefault.present = true;
            outDefault.value   = fa.FloatArgs[2];
        }
    }
}

// ---- public entry point -------------------------------------------------

bool BuildMaterialSchemaFromReflection(slang::IGlobalSession* globalSession,
                                       slang::IComponentType* linkedProgram,
                                       const std::string&     materialTypeName,
                                       MaterialSchema&        outSchema)
{
    if (!linkedProgram || !globalSession)
        return false;

    slang::ProgramLayout* programLayout = linkedProgram->getLayout(0, nullptr);
    if (!programLayout)
    {
        YG_CORE_ERROR("Slang reflection: linkedProgram->getLayout() is null");
        return false;
    }

    slang::TypeReflection* matType = programLayout->findTypeByName(materialTypeName.c_str());
    if (!matType)
    {
        YG_CORE_ERROR("Slang reflection: type '{0}' not found in linked program", materialTypeName);
        return false;
    }

    slang::TypeLayoutReflection* matLayout = programLayout->getTypeLayout(matType);
    if (!matLayout)
    {
        YG_CORE_ERROR("Slang reflection: getTypeLayout('{0}') returned null", materialTypeName);
        return false;
    }

    unsigned int   fieldCount = matLayout->getFieldCount();
    MaterialSchema schema;

    uint32_t stride = 0;
    for (unsigned int i = 0; i < fieldCount; ++i)
    {
        slang::VariableLayoutReflection* fieldVarLayout = matLayout->getFieldByIndex(i);
        if (!fieldVarLayout)
            continue;

        slang::VariableReflection* fieldVar = fieldVarLayout->getVariable();
        if (!fieldVar)
            continue;

        const char* nameC = fieldVar->getName();
        if (!nameC)
            continue;
        std::string name = nameC;

        slang::TypeLayoutReflection* fieldTypeLayout = fieldVarLayout->getTypeLayout();

        MaterialSchema::FieldKind kind;
        if (!ResolveFieldKind(fieldTypeLayout, kind))
        {
            YG_CORE_WARN("Slang reflection: field '{0}.{1}' has unsupported kind, skipping", materialTypeName, name);
            continue;
        }

        size_t offset = fieldVarLayout->getOffset(SLANG_PARAMETER_CATEGORY_UNIFORM);
        stride += fieldTypeLayout->getSize();

        std::vector<MaterialSchema::FieldAttribute> attrs;
        bool                                        taggedAsTexture = false;
        DecodedDefault                              decodedDefault;
        CollectFieldAttributes(globalSession, fieldVar, attrs, taggedAsTexture, decodedDefault);

        // [Texture] attribute on a uint field upgrades it to TextureSlot
        // for runtime packing (4-byte index). Editor still sees the same
        // attribute via Attributes[] and renders a drop target accordingly.
        if (taggedAsTexture && kind == MaterialSchema::FieldKind::Uint)
            kind = MaterialSchema::FieldKind::TextureSlot;

        schema.AddField(name, static_cast<uint32_t>(offset), kind);

        for (auto& a : attrs)
            schema.AddAttribute(name, std::move(a));

        if (decodedDefault.present)
        {
            std::visit(
                [&](auto&& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::monostate>)
                        return;
                    else
                        schema.SetDefault(name, MaterialSchema::DefaultValue{ v });
                },
                decodedDefault.value);
        }
    }

    if (stride == 0)
    {
        YG_CORE_WARN("Slang reflection: '{0}' yielded no usable fields", materialTypeName);
        return false;
    }

    schema.Build(stride);
    outSchema = std::move(schema);
    return true;
}

} // namespace Yogi
