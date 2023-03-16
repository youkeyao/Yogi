#include "backends/renderer/opengl/opengl_shader.h"
#include "runtime/core/application.h"
#include "backends/renderer/opengl/opengl_context.h"

namespace Yogi {

    static ShaderDataType spirv_type_to_shader_data_type(spirv_cross::SPIRType stype)
    {
        switch (stype.basetype) {
            case spirv_cross::SPIRType::Float:
                if (stype.vecsize == 1) return ShaderDataType::Float;
                else if (stype.vecsize == 2) return ShaderDataType::Float2;
                else if (stype.vecsize == 3) return ShaderDataType::Float3;
                else if (stype.vecsize == 4) return ShaderDataType::Float4;
            case spirv_cross::SPIRType::Int:
                if (stype.vecsize == 1) return ShaderDataType::Int;
                else if (stype.vecsize == 2) return ShaderDataType::Int2;
                else if (stype.vecsize == 3) return ShaderDataType::Int3;
                else if (stype.vecsize == 4) return ShaderDataType::Int4;
            case spirv_cross::SPIRType::Boolean:
                if (stype.vecsize == 1) return ShaderDataType::Bool;
        }
        YG_CORE_ASSERT(false, "Unknown spirv type!");
        return ShaderDataType::None;
    }

    Ref<Shader> Shader::create(const std::string& name, const std::vector<std::string>& types)
    {
        return CreateRef<OpenGLShader>(name, types);
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

    OpenGLShader::OpenGLShader(const std::string& name, const std::vector<std::string>& types)
    {
        m_name = name;

        GLuint program = glCreateProgram();
        std::vector<GLuint> shader_ids;
        for (auto type : types) {
            std::vector<uint32_t> shader_bin = read_file(YG_SHADER_DIR + name + "." + type);
            spirv_cross::CompilerGLSL glsl(std::move(shader_bin));
            glsl.set_common_options({330});
            if (type == "vert") {
                reflect_vertex(glsl);
            }
            reflect_uniform_buffer(glsl);
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
        buffer.resize(in.tellg() / sizeof(uint32_t));
        in.seekg(0, std::ios::beg);
        in.read((char*)buffer.data(), buffer.size() * sizeof(uint32_t));
        in.close();

        return buffer;
    }

    void OpenGLShader::reflect_vertex(const spirv_cross::CompilerGLSL& compiler)
    {
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        std::map<uint32_t, spirv_cross::Resource*> stage_inputs;
        for (auto& stage_input : resources.stage_inputs) {
            stage_inputs[compiler.get_decoration(stage_input.id, spv::DecorationLocation)] = &stage_input;
        }
        for (auto& [location, p_stage_input] : stage_inputs) {
            auto& stage_input = *p_stage_input;
            const auto& input_type = compiler.get_type(stage_input.base_type_id);

            m_vertex_layout.add_element({ spirv_type_to_shader_data_type(input_type), stage_input.name });
        }
    }

    void OpenGLShader::reflect_uniform_buffer(const spirv_cross::CompilerGLSL& compiler)
    {
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        for (auto& uniform_buffer : resources.uniform_buffers) {
            auto &uniform_type = compiler.get_type(uniform_buffer.type_id);
            uint32_t binding = compiler.get_decoration(uniform_buffer.id, spv::DecorationBinding);

            ShaderUniformLayout uniform_layout;
            for (int32_t i = 0; i < uniform_type.member_types.size(); i ++) {
                const auto& member_type = compiler.get_type(uniform_type.member_types[i]);
                uniform_layout.add_element({ spirv_type_to_shader_data_type(member_type), compiler.get_member_name(uniform_type.self, i) });
            }
            m_uniform_layouts[binding] = uniform_layout;
        }
    }

    void OpenGLShader::bind() const
    {
        glUseProgram(m_renderer_id);

        OpenGLContext* context = (OpenGLContext*)Application::get().get_window().get_context();
        context->set_vertex_layout(this);
    }

    void OpenGLShader::unbind() const
    {
        glUseProgram(0);
    }

}