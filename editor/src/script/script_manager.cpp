#include "script/script_manager.h"
#include "reflect/component_manager.h"

namespace Yogi {

sol::state                         ScriptManager::s_state{};
std::map<std::string, std::string> ScriptManager::s_scripts{};

void ScriptManager::init(const std::string &dir_path)
{
    // s_state.open_libraries(sol::lib::base);
    if (std::filesystem::is_directory(dir_path)) {
        for (auto &directory_entry : std::filesystem::directory_iterator(dir_path)) {
            const auto &path = directory_entry.path();
            std::string filename = path.stem().string();
            std::string extension = path.extension().string();

            if (directory_entry.is_directory()) {
                init(path.string());
            } else if (extension == ".lua") {
                s_state.script_file(path.string());
            }
        }
    }

    // Register components & systems
    for (const auto &[key, value] : s_state.globals()) {
        std::string name = key.as<std::string>();
        if (value.is<sol::table>()) {
            sol::table table = value.as<sol::table>();

            std::string   component_name = key.as<std::string>();
            ComponentType component_type;

            for (const auto &pair : table) {
                std::string key = pair.first.as<std::string>();
                sol::object value = pair.second;

                switch (value.get_type()) {
                case sol::type::boolean:
                    component_type.m_fields[key] = { component_type.m_size, typeid(bool).hash_code() };
                    component_type.m_size += sizeof(bool);
                    break;
                case sol::type::number:
                    component_type.m_fields[key] = { component_type.m_size, typeid(float).hash_code() };
                    component_type.m_size += sizeof(float);
                    break;
                case sol::type::string:
                    component_type.m_fields[key] = { component_type.m_size, typeid(std::string).hash_code() };
                    component_type.m_size += sizeof(std::string);
                    break;
                default:
                    YG_CORE_ERROR("ScriptManager: Unknown type!");
                    break;
                }
            }

            ComponentManager::register_component(component_name, component_type);
        }
    }
}

void ScriptManager::clear()
{
    s_scripts.clear();
}

}  // namespace Yogi
