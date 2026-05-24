#include "Resources/AssetManager/Serializer/ShaderSerializer.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

namespace Yogi
{

class FileIncluder : public glslang::TShader::Includer
{
public:
    FileIncluder(std::vector<std::filesystem::path> searchDirs) : m_searchDirs(std::move(searchDirs)) {}

    IncludeResult* includeLocal(const char* headerName,
                                const char* /*includerName*/,
                                size_t /*inclusionDepth*/) override
    {
        for (const auto& dir : m_searchDirs)
        {
            std::filesystem::path fullPath = dir / std::filesystem::path(headerName);
            std::ifstream         file(fullPath, std::ios::binary);
            if (!file)
                continue;

            file.seekg(0, std::ios::end);
            size_t length = file.tellg();
            file.seekg(0, std::ios::beg);

            char* data = new char[length];
            file.read(data, length);

            return new IncludeResult(fullPath.string(), data, length, data);
        }
        return nullptr;
    }

    void releaseInclude(IncludeResult* result) override
    {
        if (result)
        {
            delete[] static_cast<char*>(result->userData);
            delete result;
        }
    }

private:
    std::vector<std::filesystem::path> m_searchDirs;
};

static std::string MakePreambleFromKey(const std::string& key)
{
    size_t sepPos = key.find("::");
    if (sepPos == std::string::npos)
        return {};

    std::string preamble;
    size_t      cursor = sepPos + 2;
    while (cursor < key.size())
    {
        size_t comma = key.find(',', cursor);
        size_t end   = (comma == std::string::npos) ? key.size() : comma;

        size_t equals = key.find('=', cursor);
        if (equals != std::string::npos && equals < end)
        {
            preamble.append("#define ");
            preamble.append(key, cursor, equals - cursor);
            preamble.append(" ");
            preamble.append(key, equals + 1, end - equals - 1);
            preamble.append("\n");
        }
        else
        {
            // Bare token with no value: define as 1
            preamble.append("#define ");
            preamble.append(key, cursor, end - cursor);
            preamble.append(" 1\n");
        }

        if (comma == std::string::npos)
            break;
        cursor = comma + 1;
    }
    return preamble;
}

static std::string StripMacroSuffix(const std::string& key)
{
    size_t sepPos = key.find("::");
    return (sepPos == std::string::npos) ? key : key.substr(0, sepPos);
}

std::vector<uint8_t> CompileGlslToSpirv(const std::vector<uint8_t>& glslBinary,
                                        EShLanguage                 shaderStage,
                                        const std::string&          key)
{
    std::string source;
    source.assign(reinterpret_cast<const char*>(glslBinary.data()), glslBinary.size());
    const char* shaderStrings[1] = { source.c_str() };

    glslang::TShader shader(shaderStage);
    shader.setEnvInput(glslang::EShSourceGlsl, shaderStage, glslang::EShClientVulkan, 460);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_4);
    shader.setStrings(shaderStrings, 1);

    std::string preamble = MakePreambleFromKey(key);
    if (!preamble.empty())
        shader.setPreamble(preamble.c_str());

    int         clientInputSemanticsVersion = 100;
    EShMessages messages                    = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    std::filesystem::path shaderDir = std::filesystem::path(StripMacroSuffix(key)).parent_path();
    FileIncluder          includer({ shaderDir, "EngineInclude" });

    if (!shader.parse(GetDefaultResources(), 460, false, messages, includer))
    {
        std::string log   = shader.getInfoLog();
        std::string debug = shader.getInfoDebugLog();
        YG_CORE_ERROR("{0} GLSL parse error:\n{1}\n{2}", key, log, debug);
        return {};
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages))
    {
        std::string log   = program.getInfoLog();
        std::string debug = program.getInfoDebugLog();
        YG_CORE_ERROR("{0} GLSL link error:\n{1}\n{2}", key, log, debug);
        return {};
    }

    const glslang::TIntermediate* intermediate = program.getIntermediate(shaderStage);
    if (!intermediate)
    {
        YG_CORE_ERROR("{0} Failed to get intermediate representation after linking.", key);
        return {};
    }

    std::vector<unsigned int> spirv;
    spv::SpvBuildLogger       logger;
    glslang::GlslangToSpv(*intermediate, spirv, &logger);

    std::string loggerMsg = logger.getAllMessages();
    if (!loggerMsg.empty())
    {
        YG_CORE_WARN("{0} GLSL->SPIRV validator/warnings:\n{1}", key, loggerMsg);
    }

    std::vector<uint8_t> out;
    out.resize(spirv.size() * sizeof(unsigned int));
    static_assert(sizeof(unsigned int) == 4, "This code expects unsigned int to be 4 bytes");
    memcpy(out.data(), spirv.data(), out.size());

    return out;
}

ShaderSerializer::ShaderSerializer()
{
    glslang::InitializeProcess();
}

ShaderSerializer::~ShaderSerializer()
{
    glslang::FinalizeProcess();
}

Owner<ShaderDesc> ShaderSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    std::filesystem::path path(StripMacroSuffix(key));
    if (path.extension() == ".vert")
        return Owner<ShaderDesc>::Create(ShaderStage::Vertex, CompileGlslToSpirv(binary, EShLangVertex, key));
    else if (path.extension() == ".frag")
        return Owner<ShaderDesc>::Create(ShaderStage::Fragment, CompileGlslToSpirv(binary, EShLangFragment, key));
    else if (path.extension() == ".mesh")
        return Owner<ShaderDesc>::Create(ShaderStage::Mesh, CompileGlslToSpirv(binary, EShLangMesh, key));
    else if (path.extension() == ".task")
        return Owner<ShaderDesc>::Create(ShaderStage::Task, CompileGlslToSpirv(binary, EShLangTask, key));
    else if (path.extension() == ".comp")
        return Owner<ShaderDesc>::Create(ShaderStage::Compute, CompileGlslToSpirv(binary, EShLangCompute, key));
    return nullptr;
}

std::vector<uint8_t> ShaderSerializer::Serialize(const WRef<ShaderDesc>& asset, const std::string& key)
{
    return {};
}

} // namespace Yogi
