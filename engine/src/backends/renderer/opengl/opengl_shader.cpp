#include "backends/renderer/opengl/opengl_shader.h"
#include <spirv_glsl.hpp>

namespace Yogi {

    Ref<Shader> Shader::create(const std::string& name, const std::vector<std::string>& types)
    {
        return CreateRef<OpenGLShader>(name);
    }

    static GLenum shader_type_from_string(const std::string& type)
    {
        if (type == "vert") {
            return GL_VERTEX_SHADER;
        }
        else if (type == "frag") {
            return GL_FRAGMENT_SHADER;
        }

        YG_CORE_ASSERT(false, "unknown shader type");
        return 0;
    }

    OpenGLShader::OpenGLShader(const std::string& name, const std::vector<std::string>& types) : m_name(name)
    {
        YG_PROFILE_FUNCTION();

        GLuint program = glCreateProgram();
        std::vector<GLuint> shader_ids;
        for (auto type : types) {
            std::vector<uint32_t> shader_bin = read_file(YG_SHADER_DIR + name + "." + type);
            spirv_cross::CompilerGLSL glsl(std::move(shader_bin));
            glsl.set_common_options({330});
            std::string shader_source = glsl.compile();
            GLuint shader_id = shader_ids.emplace_back(glCreateShader(shader_type_from_string(type)));
            const GLchar *source_cstr = shader_source.c_str();
            glShaderSource(shader_id, 1, &source_cstr, 0);
            glCompileShader(shader_id);
            GLint is_compiled = 0;
            glGetShaderiv(shader_id, GL_COMPILE_STATUS, &is_compiled);
            if (is_compiled == GL_FALSE) {
                GLint max_length = 0;
                glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &max_length);

                std::vector<GLchar> info_log(max_length);
                glGetShaderInfoLog(shader_id, max_length, &max_length, &info_log[0]);
                
                glDeleteShader(shader_id);

                YG_CORE_ERROR("{0}", info_log.data());
                YG_CORE_ASSERT(false, type + " Shader compile failure!");
            }
            glAttachShader(program, shader_id);
        }
        glLinkProgram(program);
        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
        if (isLinked == GL_FALSE) {
            GLint max_length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);

            std::vector<GLchar> info_log(max_length);
            glGetProgramInfoLog(program, max_length, &max_length, &info_log[0]);
            
            glDeleteProgram(program);
            for (auto id : shader_ids) {
                glDetachShader(program, id);
                glDeleteShader(id);
            }
            
            YG_CORE_ERROR("{0}", info_log.data());
            YG_CORE_ASSERT(false, "Shader link failure!");
        }
        for (auto id : shader_ids) {
            glDetachShader(program, id);
            glDeleteShader(id);
        }
        m_renderer_id = program;
    }

    OpenGLShader::~OpenGLShader()
    {
        YG_PROFILE_FUNCTION();

        glDeleteProgram(m_renderer_id);
    }

    std::vector<uint32_t> OpenGLShader::read_file(const std::string& filepath)
    {
        std::vector<uint32_t> buffer;
        std::ifstream in(filepath, std::ios::ate | std::ios::binary);

        if (!in.is_open()) {
            YG_CORE_ERROR("Could not open file '{0}'!", filepath);
            return buffer;
        }

        in.seekg(0, std::ios::end);
        buffer.resize(in.tellg() / 4);
        in.seekg(0, std::ios::beg);
        in.read((char*)buffer.data(), buffer.size() * 4);
        in.close();

        return buffer;
    }

    void OpenGLShader::bind() const
    {
        YG_PROFILE_FUNCTION();

        glUseProgram(m_renderer_id);
    }

    void OpenGLShader::unbind() const
    {
        YG_PROFILE_FUNCTION();

        glUseProgram(0);
    }

}