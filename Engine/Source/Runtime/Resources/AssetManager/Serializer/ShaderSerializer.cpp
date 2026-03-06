#include "Resources/AssetManager/Serializer/ShaderSerializer.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

namespace Yogi
{

std::vector<uint8_t> CompileGlslToSpirv(const std::vector<uint8_t>& glslBinary, EShLanguage shaderStage)
{
    std::string source;
    source.assign(reinterpret_cast<const char*>(glslBinary.data()), glslBinary.size());
    const char* shaderStrings[1] = { source.c_str() };

    glslang::TShader shader(shaderStage);
    shader.setEnvInput(glslang::EShSourceGlsl, shaderStage, glslang::EShClientVulkan, 460);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_4);
    shader.setStrings(shaderStrings, 1);

    int         clientInputSemanticsVersion = 100;
    EShMessages messages                    = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    if (!shader.parse(GetDefaultResources(), 460, false, messages))
    {
        std::string log   = shader.getInfoLog();
        std::string debug = shader.getInfoDebugLog();
        YG_CORE_ERROR("GLSL parse error:\n{0}\n{1}", log, debug);
        return {};
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages))
    {
        std::string log   = program.getInfoLog();
        std::string debug = program.getInfoDebugLog();
        YG_CORE_ERROR("GLSL link error:\n{0}\n{1}", log, debug);
        return {};
    }

    const glslang::TIntermediate* intermediate = program.getIntermediate(shaderStage);
    if (!intermediate)
    {
        YG_CORE_ERROR("Failed to get intermediate representation after linking.");
        return {};
    }

    std::vector<unsigned int> spirv;
    spv::SpvBuildLogger       logger;
    glslang::GlslangToSpv(*intermediate, spirv, &logger);

    std::string loggerMsg = logger.getAllMessages();
    if (!loggerMsg.empty())
    {
        YG_CORE_WARN("GLSL->SPIRV validator/warnings:\n{0}", loggerMsg);
    }

    std::vector<uint8_t> out;
    out.resize(spirv.size() * sizeof(unsigned int));
    static_assert(sizeof(unsigned int) == 4, "This code expects unsigned int to be 4 bytes");
    memcpy(out.data(), spirv.data(), out.size());

    return out;
}

ShaderSerializer::ShaderSerializer() { glslang::InitializeProcess(); }

ShaderSerializer::~ShaderSerializer() { glslang::FinalizeProcess(); }

Handle<ShaderDesc> ShaderSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    std::filesystem::path path(key);
    if (path.extension() == ".vert")
        return Handle<ShaderDesc>::Create(ShaderStage::Vertex, CompileGlslToSpirv(binary, EShLangVertex));
    else if (path.extension() == ".frag")
        return Handle<ShaderDesc>::Create(ShaderStage::Fragment, CompileGlslToSpirv(binary, EShLangFragment));
    else if (path.extension() == ".mesh")
        return Handle<ShaderDesc>::Create(ShaderStage::Mesh, CompileGlslToSpirv(binary, EShLangMesh));
    else if (path.extension() == ".task")
        return Handle<ShaderDesc>::Create(ShaderStage::Task, CompileGlslToSpirv(binary, EShLangTask));
    return nullptr;
}

std::vector<uint8_t> ShaderSerializer::Serialize(const Ref<ShaderDesc>& asset, const std::string& key) { return {}; }

} // namespace Yogi
