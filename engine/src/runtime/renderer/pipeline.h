#pragma once

#include <glm/glm.hpp>

namespace Yogi {

enum class ShaderDataType : uint8_t { None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool };

static uint32_t shader_data_type_size(ShaderDataType type)
{
    switch (type) {
    case ShaderDataType::Float:
        return sizeof(float);
    case ShaderDataType::Float2:
        return sizeof(float) * 2;
    case ShaderDataType::Float3:
        return sizeof(float) * 3;
    case ShaderDataType::Float4:
        return sizeof(float) * 4;
    case ShaderDataType::Mat3:
        return sizeof(float) * 3 * 3;
    case ShaderDataType::Mat4:
        return sizeof(float) * 4 * 4;
    case ShaderDataType::Int:
        return sizeof(int);
    case ShaderDataType::Int2:
        return sizeof(int) * 2;
    case ShaderDataType::Int3:
        return sizeof(int) * 3;
    case ShaderDataType::Int4:
        return sizeof(int) * 4;
    case ShaderDataType::Bool:
        return sizeof(bool);
    }
    YG_CORE_ASSERT(false, "unknown ShaderDataType!");
    return 0;
}

static uint32_t shader_data_type_count(ShaderDataType type)
{
    switch (type) {
    case ShaderDataType::Int:
    case ShaderDataType::Float:
    case ShaderDataType::Bool:
        return 1;
    case ShaderDataType::Int2:
    case ShaderDataType::Float2:
        return 2;
    case ShaderDataType::Int3:
    case ShaderDataType::Float3:
        return 3;
    case ShaderDataType::Int4:
    case ShaderDataType::Float4:
        return 4;
    case ShaderDataType::Mat3:
        return 3 * 3;
    case ShaderDataType::Mat4:
        return 4 * 4;
    }
    YG_CORE_ASSERT(false, "unknown ShaderDataType!");
    return 0;
}

struct ShaderElement
{
    ShaderDataType type;
    std::string    name;
    uint32_t       offset;
    uint32_t       size;
    uint32_t       count;

    ShaderElement(ShaderDataType type, const std::string &name)
        : type(type), name(name), offset(0), size(shader_data_type_size(type)), count(shader_data_type_count(type))
    {
    }
};

class PipelineLayout
{
public:
    PipelineLayout() {}
    PipelineLayout(const std::initializer_list<ShaderElement> &elements)
    {
        for (auto element : elements)
            add_element(element);
    }

    inline uint32_t                          get_stride() const { return m_stride; }
    inline const std::vector<ShaderElement> &get_elements() const { return m_elements; }

    std::vector<ShaderElement>::const_iterator begin() const { return m_elements.begin(); }
    std::vector<ShaderElement>::const_iterator end() const { return m_elements.end(); }

    void add_element(const ShaderElement &element)
    {
        m_elements.push_back(element);
        ShaderElement &se = m_elements.back();
        se.offset = m_stride;
        m_stride += se.size;
    }

private:
    std::vector<ShaderElement> m_elements;
    uint32_t                   m_stride = 0;
};

class Pipeline
{
public:
    virtual ~Pipeline() = default;

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    void               set_name(const std::string &name) { m_name = name; }
    const std::string &get_name() const { return m_name; }

    const PipelineLayout &get_vertex_layout() const { return m_vertex_layout; }
    const PipelineLayout &get_output_layout() const { return m_output_layout; }

    static Ref<Pipeline> create(const std::string &name, const std::vector<std::string> &types = { "vert", "frag" });

protected:
    PipelineLayout m_vertex_layout;
    PipelineLayout m_output_layout;
    std::string    m_name;
};

}  // namespace Yogi