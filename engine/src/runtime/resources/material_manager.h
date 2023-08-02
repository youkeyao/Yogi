#pragma once

#include "runtime/renderer/material.h"

namespace Yogi {

    class MaterialManager
    {
    public:
        static void init(const std::string& dir_path);
        static void clear();
        static void add_material(const std::string& key, const Ref<Material>& material);
        static void remove_material(const Ref<Material>& material);
        static const Ref<Material>& get_material(const std::string& key);
        static void each_material(std::function<void(const Ref<Material>&)> func);

        static void save_material(const std::string& path, const Ref<Material>& material);
        static Ref<Material> load_material(const std::string& path);
    private:
        static std::map<std::string, Ref<Material>> s_materials;
        static std::string serialize_material(const Ref<Material>& material);
    };

}