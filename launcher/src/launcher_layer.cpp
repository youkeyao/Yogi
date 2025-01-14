#include "launcher_layer.h"
#include "reflect/component_manager.h"
#include "reflect/system_manager.h"
#include "reflect/scene_manager.h"
#include "script/script_manager.h"

LauncherLayer::LauncherLayer(const std::string &file_path) : Layer("Launcher"), m_file_path(file_path) {}

void LauncherLayer::on_attach()
{
    Yogi::ComponentManager::init();
    Yogi::SystemManager::init();
    Yogi::ScriptManager::init();

    Yogi::AssetManager::init_project(m_file_path);
    Yogi::ScriptManager::init_project(m_file_path);

    std::ifstream in(m_file_path + "/main.yg", std::ios::in);
    if (!in) {
        YG_CORE_WARN("Could not open file '{0}'!", m_file_path + "/main.yg");
        return;
    }
    std::string json;
    in.seekg(0, std::ios::end);
    json.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(json.data(), json.size());
    in.close();
    m_scene = Yogi::SceneManager::deserialize_scene(json);
}

void LauncherLayer::on_detach()
{
    m_scene.reset();
    Yogi::ComponentManager::clear();
    Yogi::SystemManager::clear();
    Yogi::ScriptManager::clear();
}

void LauncherLayer::on_update(Yogi::Timestep ts)
{
    m_scene->on_update(ts);
}

void LauncherLayer::on_event(Yogi::Event &event)
{
    m_scene->on_event(event);
}