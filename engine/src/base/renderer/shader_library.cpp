#include "base/renderer/shader_library.h"
#include "base/renderer/renderer.h"

namespace Yogi {

    void ShaderLibrary::add(Ref<Shader> shader) {
        auto& name = shader->get_name();
        YG_CORE_ASSERT(!exists(name), "shader already exists");
        m_shaders[name] = std::move(shader);
    }

    Ref<Shader> ShaderLibrary::load(const std::string& filepath) {
        Ref<Shader> shader = Shader::create(filepath);
        add(shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::load(const std::string& name, const std::string& vertex_source, const std::string& fragment_source) {
        Ref<Shader> shader = Shader::create(name, vertex_source, fragment_source);
        add(shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::get(const std::string& name) {
        YG_CORE_ASSERT(exists(name), "shader not found");
        return m_shaders[name];
    }

    bool ShaderLibrary::exists(const std::string& name) const {
        return m_shaders.find(name) != m_shaders.end();
    }

}