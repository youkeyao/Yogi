#include "renderer/shader.h"
#include "renderer/renderer.h"
#include "platform/opengl/opengl_shader.h"

namespace hazel {

    Ref<Shader> Shader::create(const std::string& filepath)
    {
        switch (Renderer::get_api()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None not supported");
                return nullptr;

            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLShader>(filepath);
        }

        HZ_CORE_ASSERT(false, "unknown RendererAPI");
        return nullptr;
    }

    Ref<Shader> Shader::create(const std::string& name, const std::string& vertex_source, const std::string& fragment_source)
    {
        switch (Renderer::get_api()) {
            case RendererAPI::API::None:
                HZ_CORE_ASSERT(false, "RendererAPI::None not supported");
                return nullptr;

            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLShader>(name, vertex_source, fragment_source);
        }

        HZ_CORE_ASSERT(false, "unknown RendererAPI");
        return nullptr;
    }

    void ShaderLibrary::add(Ref<Shader> shader) {
        auto& name = shader->get_name();
        HZ_CORE_ASSERT(!exists(name), "shader already exists");
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
        HZ_CORE_ASSERT(exists(name), "shader not found");
        return m_shaders[name];
    }

    bool ShaderLibrary::exists(const std::string& name) const {
        return m_shaders.find(name) != m_shaders.end();
    }

}