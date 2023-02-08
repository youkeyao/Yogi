#pragma once

namespace hazel {

    enum class ShaderDataType : uint8_t {
        None = 0,
        Float, Float2, Float3, Float4,
        Mat3, Mat4,
        Int, Int2, Int3, Int4,
        Bool
    };

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
        HZ_CORE_ASSERT(false, "unknown ShaderDataType!");
        return 0;
    }

    struct BufferElement
    {
        ShaderDataType type;
        std::string name;
        uint32_t size;
        uint32_t offset;
        bool normalized = false;

        BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
            : type(type), name(name), size(shader_data_type_size(type)), offset(0), normalized(normalized) {}

        uint32_t get_component_count() const
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
            HZ_CORE_ASSERT(false, "unknown ShaderDataType!");
            return 0;
        }
    };

    class BufferLayout
    {
    public:
        BufferLayout() {}
        BufferLayout(const std::initializer_list<BufferElement>& elements) : m_elements(elements)
        {
            calculate_offsets_and_stride();
        }

        inline uint32_t get_stride() const { return m_stride; }
        inline const std::vector<BufferElement>& get_elements() const { return m_elements; }

        std::vector<BufferElement>::const_iterator begin() const { return m_elements.begin(); }
        std::vector<BufferElement>::const_iterator end() const { return m_elements.end(); }        

    private:
        void calculate_offsets_and_stride()
        {
            uint32_t offset = 0;
            m_stride = 0;

            for (auto& element : m_elements) {
                element.offset = offset;
                offset += element.size;
                m_stride += element.size;
            }
        };

        std::vector<BufferElement> m_elements;
        uint32_t m_stride = 0;
    };

    class VertexBuffer
    {
    public:
        virtual ~VertexBuffer() {}

        virtual void bind() const = 0;
        virtual void unbind() const = 0;

        virtual const BufferLayout& get_layout() const = 0;
        virtual void set_layout(const BufferLayout& layout) = 0;

        virtual void set_data(const void* data, uint32_t size) = 0;

        static Ref<VertexBuffer> create(float* vertices, uint32_t size, bool is_static = true);
    };

    class IndexBuffer
    {
    public:
        virtual ~IndexBuffer() {}

        virtual void bind() const = 0;
        virtual void unbind() const = 0;

        virtual uint32_t get_count() const = 0;

        static Ref<IndexBuffer> create(uint32_t* indices, uint32_t count);
    };

}