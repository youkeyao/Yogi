#pragma once

#include "Core/Singleton.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/IShaderResourceBinding.h"
#include "Renderer/MaterialSchema.h"

namespace Yogi
{

class YG_API SlangShaderCompiler : public Singleton<SlangShaderCompiler>
{
    friend class Singleton<SlangShaderCompiler>;

public:
    static std::vector<uint8_t> Compile(const std::string&                                      slangSource,
                                        const std::filesystem::path&                            sourcePath,
                                        const std::string&                                      entryPoint,
                                        ShaderStage                                             stage,
                                        const std::vector<std::pair<std::string, std::string>>& macros);

    static std::vector<uint8_t> CompileSpecialized(const std::string&           slangSource,
                                                   const std::filesystem::path& sourcePath,
                                                   const std::string&           entryPoint,
                                                   ShaderStage                  stage,
                                                   const std::string&           materialTypeName,
                                                   const std::string&           shaderKey,
                                                   const std::vector<std::pair<std::string, std::string>>& macros);

    static MaterialSchema ReflectMaterialSchema(const std::string&           slangSource,
                                                const std::filesystem::path& sourcePath,
                                                const std::string&           materialTypeName);

protected:
    SlangShaderCompiler();
    ~SlangShaderCompiler();

private:
    static SlangShaderCompiler* s_instance;

    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace Yogi
