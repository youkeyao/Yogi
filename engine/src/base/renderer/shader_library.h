#pragma once

#include "base/renderer/shader.h"

namespace Yogi {

    class ShaderLibrary
    {
    public:
        void add(Ref<Shader>);

        Ref<Shader> load(const std::string& filepath);
        Ref<Shader> load(const std::string& name, const std::string& vertex_source, const std::string& fragment_source);

        Ref<Shader> get(const std::string& name);

        bool exists(const std::string& name) const;

    private:
        std::unordered_map<std::string, Ref<Shader>> m_shaders;
    };

}