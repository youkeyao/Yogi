#include "Resources/AssetManager/Serializer/ShaderSerializer.h"
#include "Resources/AssetManager/Serializer/SlangShaderCompiler.h"

namespace Yogi
{

static std::vector<std::pair<std::string, std::string>> ParseMacrosFromKey(const std::string& key)
{
    std::vector<std::pair<std::string, std::string>> out;
    size_t                                           sepPos = key.find("::");
    if (sepPos == std::string::npos)
        return out;

    size_t cursor = sepPos + 2;
    while (cursor < key.size())
    {
        size_t comma = key.find(',', cursor);
        size_t end   = (comma == std::string::npos) ? key.size() : comma;

        size_t equals = key.find('=', cursor);
        if (equals != std::string::npos && equals < end)
        {
            out.emplace_back(key.substr(cursor, equals - cursor), key.substr(equals + 1, end - equals - 1));
        }
        else
        {
            out.emplace_back(key.substr(cursor, end - cursor), "1");
        }

        if (comma == std::string::npos)
            break;
        cursor = comma + 1;
    }
    return out;
}

static std::string StripMacroSuffix(const std::string& key)
{
    size_t sepPos = key.find("::");
    return (sepPos == std::string::npos) ? key : key.substr(0, sepPos);
}

struct SlangStageMapping
{
    ShaderStage Stage;
    const char* EntryPoint;
};

static bool ResolveSlangStage(const std::filesystem::path& path, SlangStageMapping& out)
{
    std::string stem   = path.stem().string(); // strips trailing ".slang"
    auto        dot    = stem.find_last_of('.');
    std::string subExt = (dot == std::string::npos) ? std::string{} : stem.substr(dot + 1);

    if (subExt == "vs" || subExt == "vert")
    {
        out = { ShaderStage::Vertex, "vsMain" };
        return true;
    }
    if (subExt == "fs" || subExt == "frag")
    {
        out = { ShaderStage::Fragment, "fsMain" };
        return true;
    }
    if (subExt == "cs" || subExt == "comp")
    {
        out = { ShaderStage::Compute, "csMain" };
        return true;
    }
    if (subExt == "ms" || subExt == "mesh")
    {
        out = { ShaderStage::Mesh, "msMain" };
        return true;
    }
    if (subExt == "as" || subExt == "task")
    {
        out = { ShaderStage::Task, "asMain" };
        return true;
    }
    if (subExt == "gs")
    {
        out = { ShaderStage::Geometry, "gsMain" };
        return true;
    }
    return false;
}

static Owner<ShaderDesc> CompileSlangAsset(const std::vector<uint8_t>& binary, const std::string& key)
{
    std::filesystem::path path(StripMacroSuffix(key));

    SlangStageMapping mapping{};
    if (!ResolveSlangStage(path, mapping))
    {
        YG_CORE_ERROR("Slang: cannot infer stage from '{0}'. Expected '<name>.<vs|fs|cs|ms|as|gs>.slang'.",
                      path.string());
        return nullptr;
    }

    std::string source(reinterpret_cast<const char*>(binary.data()), binary.size());
    auto        macros = ParseMacrosFromKey(key);

    constexpr const char* kSpecializeMarker = "__SPECIALIZE_M";
    std::string           specializeType;
    for (auto it = macros.begin(); it != macros.end();)
    {
        if (it->first == kSpecializeMarker)
        {
            specializeType = it->second;
            it             = macros.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (!specializeType.empty())
    {
        auto code = SlangShaderCompiler::CompileSpecialized(
            source, path, mapping.EntryPoint, mapping.Stage, specializeType, key, macros);
        if (code.empty())
            return nullptr;

        return Owner<ShaderDesc>::Create(mapping.Stage, std::move(code));
    }

    auto code = SlangShaderCompiler::Compile(source, path, mapping.EntryPoint, mapping.Stage, macros);
    if (code.empty())
        return nullptr;

    return Owner<ShaderDesc>::Create(mapping.Stage, std::move(code));
}

Owner<ShaderDesc> ShaderSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    std::filesystem::path path(StripMacroSuffix(key));
    if (path.extension() == ".slang")
        return CompileSlangAsset(binary, key);

    YG_CORE_ERROR("ShaderSerializer: unsupported shader extension '{0}' (Phase 6 dropped GLSL; use *.<stage>.slang)",
                  path.extension().string());
    return nullptr;
}

std::vector<uint8_t> ShaderSerializer::Serialize(const WRef<ShaderDesc>& asset, const std::string& key)
{
    return {};
}

} // namespace Yogi
