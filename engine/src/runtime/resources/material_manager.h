#pragma once

#include "runtime/renderer/material.h"

namespace Yogi {

    class MaterialManager
    {
    public:
        static void init();
        static void clear();
        static Ref<Material>& add_material(const std::string& name, const Ref<Material>& material);
        static const Ref<Material>& get_material(const std::string& name) { return s_materials[name]; }
    private:
        static std::map<std::string, Ref<Material>> s_materials;
    };

}