#include "platform/renderer/opengl/opengl_shader.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace hazel {

    Ref<Shader> Shader::create(const std::string& filepath)
    {
        return CreateRef<OpenGLShader>(filepath);
    }

    Ref<Shader> Shader::create(const std::string& name, const std::string& vertex_source, const std::string& fragment_source)
    {
        return CreateRef<OpenGLShader>(name, vertex_source, fragment_source);
    }

    static GLenum shader_type_from_string(const std::string& type)
    {
        if (type == "vertex") {
            return GL_VERTEX_SHADER;
        }
        else if (type == "fragment" || type == "pixel") {
            return GL_FRAGMENT_SHADER;
        }

        HZ_CORE_ASSERT(false, "unknown shader type");
        return 0;
    }

    OpenGLShader::OpenGLShader(const std::string& filepath)
    {
        HZ_PROFILE_FUNCTION();

        std::string source = read_file(filepath);
        std::unordered_map<GLenum, std::string> shader_sources = preprocess(source);
        compile(shader_sources);

        auto last_slash = filepath.find_last_of("/\\");
        last_slash = last_slash == std::string::npos ? 0 : last_slash + 1;
        auto last_dot = filepath.rfind('.');
        last_dot = last_dot == std::string::npos ? filepath.size() : last_dot;

        auto count = last_dot - last_slash;
        m_name = filepath.substr(last_slash, count);
    }

    OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc) : m_name(name)
    {
        HZ_PROFILE_FUNCTION();

        std::unordered_map<GLenum, std::string> sources;
        sources[GL_VERTEX_SHADER] = vertexSrc;
        sources[GL_FRAGMENT_SHADER] = fragmentSrc;
        compile(sources);
    }

    OpenGLShader::~OpenGLShader()
    {
        HZ_PROFILE_FUNCTION();

        glDeleteProgram(m_renderer_id);
    }

    std::string OpenGLShader::read_file(const std::string& filepath)
    {
        HZ_PROFILE_FUNCTION();

        std::string result = std::string();
        std::ifstream in(filepath, std::ios::in | std::ios::binary);

        if (!in) {
            HZ_CORE_ERROR("could not open file '{0}'", filepath);
            return result;
        }

        in.seekg(0, std::ios::end);
        result.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&result[0], result.size());
        in.close();

        return result;
    }

    std::unordered_map<GLenum, std::string> OpenGLShader::preprocess(const std::string& source)
    {
        HZ_PROFILE_FUNCTION();

        std::unordered_map<GLenum, std::string> shader_sources;

        const char* type_token = "#type";
        size_t type_token_length = strlen(type_token);
        size_t pos = source.find(type_token, 0);

        while (pos != std::string::npos) {
            size_t eol = source.find_first_of("\r\n", pos);
            HZ_CORE_ASSERT(eol != std::string::npos, "syntax error");
            size_t begin = pos + type_token_length + 1;
            std::string type = source.substr(begin, eol - begin);
            HZ_CORE_ASSERT(shader_type_from_string(type), "invalid shader type specification");

            size_t next_line_pos = source.find_first_not_of("\r\n", eol);
            pos = source.find(type_token, next_line_pos);

            shader_sources[shader_type_from_string(type)] =
                source.substr(next_line_pos, pos - (next_line_pos == std::string::npos ? source.size() - 1 : next_line_pos));
        }

        return shader_sources;
    }

    void OpenGLShader::compile(const std::unordered_map<GLenum, std::string>& shader_sources)
    {
        HZ_PROFILE_FUNCTION();

        GLuint program = glCreateProgram();
        GLenum shader_ids[shader_sources.size()];
        int shader_id_index = 0;

        for (auto& kv : shader_sources) {
            GLenum type = kv.first;
            const std::string& source = kv.second;

            GLuint shader = glCreateShader(type);

            const GLchar *source_cstr = source.c_str();
            glShaderSource(shader, 1, &source_cstr, 0);

            glCompileShader(shader);

            GLint isCompiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
            if (isCompiled == GL_FALSE) {
                GLint maxLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

                std::vector<GLchar> infoLog(maxLength);
                glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);
                
                glDeleteShader(shader);

                HZ_CORE_ERROR("{0}", infoLog.data());
                HZ_CORE_ASSERT(false, "shader compilation failure!");
                return;
            }
            glAttachShader(program, shader);
            shader_ids[shader_id_index++] = shader;
        }

        glLinkProgram(program);

        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
        if (isLinked == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
            
            glDeleteProgram(program);
            for (auto id : shader_ids) {
                glDeleteShader(id);
            }
            
            HZ_CORE_ERROR("{0}", infoLog.data());
            HZ_CORE_ASSERT(false, "shader link failure!");
            return;
        }

        for (auto id : shader_ids) {
            glDetachShader(program, id);
            glDeleteShader(id);
        }

        m_renderer_id = program;
    }

    void OpenGLShader::bind() const
    {
        HZ_PROFILE_FUNCTION();

        glUseProgram(m_renderer_id);
    }

    void OpenGLShader::unbind() const
    {
        HZ_PROFILE_FUNCTION();

        glUseProgram(0);
    }

    void OpenGLShader::set_int(const std::string& name, int value) const
    {
        HZ_PROFILE_FUNCTION();

        const GLint location = glGetUniformLocation(m_renderer_id, name.c_str());
        glUniform1i(location, value);
    }
    void OpenGLShader::set_int_array(const std::string& name, int* values, uint32_t count) const
    {
        HZ_PROFILE_FUNCTION();

        const GLint location = glGetUniformLocation(m_renderer_id, name.c_str());
        glUniform1iv(location, count, values);
    }
    void OpenGLShader::set_float(const std::string& name, float value) const
    {
        HZ_PROFILE_FUNCTION();

        const GLint location = glGetUniformLocation(m_renderer_id, name.c_str());
        glUniform1f(location, value);
    }
    void OpenGLShader::set_float3(const std::string& name, const glm::vec3& value) const
    {
        HZ_PROFILE_FUNCTION();

        const GLint location = glGetUniformLocation(m_renderer_id, name.c_str());
        glUniform3f(location, value.x, value.y, value.z);
    }
    void OpenGLShader::set_float4(const std::string& name, const glm::vec4& value) const
    {
        HZ_PROFILE_FUNCTION();

        const GLint location = glGetUniformLocation(m_renderer_id, name.c_str());
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }
    void OpenGLShader::set_mat4(const std::string& name, const glm::mat4& value) const
    {
        HZ_PROFILE_FUNCTION();

        const GLint location = glGetUniformLocation(m_renderer_id, name.c_str());
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }

}