#include "runtime/resources/material_manager.h"
#include "runtime/resources/pipeline_manager.h"
#include "runtime/resources/texture_manager.h"

namespace Yogi {

    std::map<std::string, Ref<Material>> MaterialManager::s_materials{};

    void MaterialManager::init()
    {
        auto& material = add_material("checkerboard", CreateRef<Material>(PipelineManager::get_pipeline("Flat")));
        material->set_texture(0, TextureManager::get_texture("checkerboard"));
    }

    void MaterialManager::clear()
    {
        s_materials.clear();
    }

    Ref<Material>& MaterialManager::add_material(const std::string& name, const Ref<Material>& material)
    {
        s_materials[name] = material;
        return s_materials[name];
    }

}