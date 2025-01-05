#include "script/script_manager.h"
#include "reflect/component_manager.h"
#include "reflect/system_manager.h"

namespace Yogi {

std::string lua_to_string(lua_State *L, int idx)
{
    std::ostringstream str;

    int type = lua_type(L, idx);

    switch (type) {
    case LUA_TSTRING:
        str << lua_tostring(L, idx);
        break;

    case LUA_TNUMBER: {
        if (lua_isinteger(L, idx)) {
            str << lua_tointeger(L, idx);
        } else {
            str << lua_tonumber(L, idx);
        }
        break;
    }

    case LUA_TBOOLEAN:
        str << (lua_toboolean(L, idx) ? "true" : "false");
        break;

    case LUA_TNIL:
        str << "nil";
        break;

    case LUA_TTABLE: {
        str << "{ ";

        lua_pushvalue(L, idx);
        int t = lua_gettop(L);
        lua_pushnil(L);
        while (lua_next(L, t) != 0) {
            str << lua_to_string(L, -2) << ":" << lua_to_string(L, -1) << ", ";
            lua_pop(L, 1);
        }
        lua_pop(L, 1);

        str << "}";
        break;
    }
    case LUA_TFUNCTION:
        str << "function";
        break;

    case LUA_TTHREAD:
        str << "thread";
        break;

    case LUA_TUSERDATA:
        str << "userdata";
        break;

    default:
        str << "unknown_type";
        break;
    }

    return str.str();
}

int lua_print(lua_State *L)
{
    int n = lua_gettop(L);
    if (n == 0) {
        return 0;
    }

    std::string str;
    for (int i = 1; i <= n; i++) {
        if (i > 1) {
            str += "\t";
        }

        str += lua_to_string(L, i);
    }

    YG_INFO("{0}", str);
    return 0;
}

// ---------------------------------------------------------------------------------------------------

sol::state ScriptManager::s_state{};

void ScriptManager::init_usertype()
{
    s_state.new_usertype<Timestep>(
        "Timestep", sol::constructors<Timestep(float)>(),  // constructor
        "get_seconds", &Timestep::get_seconds,             // get seconds func
        "get_miliseconds", &Timestep::get_miliseconds      // get miliseconds func
    );
    s_state.new_usertype<Event>(
        "Event", sol::no_constructor,              // constructor
        "get_event_type", &Event::get_event_type,  // get event type
        "m_handled", &Event::m_handled             // check if event is handled
    );
    s_state.new_usertype<RuntimeComponent>(
        "RuntimeComponent", sol::no_constructor,  // constructor
        sol::meta_function::index,
        [](RuntimeComponent &comp, const std::string &field_name) -> sol::object {
            void *data = comp.get_data();
            auto &type = comp.get_type();
            auto &it = type.fields.find(field_name);
            YG_ASSERT(it != type.fields.end(), "RuntimeComponent get unknown field name: {0}!", field_name);
            const Field &field = it->second;
            void *field_ptr = (uint8_t *)data + field.offset;

            if (field.type_hash == typeid(int).hash_code()) {
                return sol::make_object(s_state, *static_cast<int *>(field_ptr));
            } else if (field.type_hash == typeid(float).hash_code()) {
                return sol::make_object(s_state, *static_cast<float *>(field_ptr));
            } else if (field.type_hash == typeid(std::string).hash_code()) {
                return sol::make_object(s_state, *static_cast<std::string *>(field_ptr));
            }
            return sol::nil;
        },  // get field
        sol::meta_function::new_index,
        [](RuntimeComponent &comp, const std::string &field_name, sol::object value) {
            void *data = comp.get_data();
            auto &type = comp.get_type();
            auto &it = type.fields.find(field_name);
            YG_ASSERT(it != type.fields.end(), "RuntimeComponent set unknown field name: {0}!", field_name);
            const Field &field = it->second;
            void *field_ptr = (uint8_t *)data + field.offset;

            if (field.type_hash == typeid(int).hash_code()) {
                *static_cast<int *>(field_ptr) = value.as<int>();
            } else if (field.type_hash == typeid(float).hash_code()) {
                *static_cast<float *>(field_ptr) = value.as<float>();
            } else if (field.type_hash == typeid(std::string).hash_code()) {
                *static_cast<std::string *>(field_ptr) = value.as<std::string>();
            }
        }  // set field
    );
    s_state.new_usertype<Entity>(
        "Entity", sol::no_constructor,  // constructor
        "get_component",
        [](Entity &entity, const std::string &component_name) -> RuntimeComponent {
            const auto &component_type = ComponentManager::get_component_type(component_name);
            return RuntimeComponent{ entity.get_runtime_component(component_type.type_id), &component_type };
        }  // get component
    );
    s_state.new_usertype<Scene>(
        "Scene", sol::no_constructor,            // constructor
        "create_entity", &Scene::create_entity,  // create entity
        "get_entity", &Scene::get_entity,        // get entity
        "delete_entity", &Scene::delete_entity,  // delete entity
        "each_entity", &Scene::each_entity,      // each entity
        "each_system", &Scene::each_system,      // each system
        "view_components",
        [](Scene &scene, sol::table component_names, sol::function func) {
            std::vector<uint32_t> component_types;
            for (const auto &[key, value] : component_names) {
                if (value.get_type() == sol::type::string) {
                    const ComponentType &component_type = ComponentManager::get_component_type(value.as<std::string>());
                    component_types.emplace_back(component_type.type_id);
                }
            }
            scene.view_runtime_components(component_types, func);
        }  // view components
    );
    lua_register(s_state, "print", lua_print);
}

void ScriptManager::init(const std::string &dir_path)
{
    init_usertype();
    // register component and system
    s_state.set_function("register_component", [](const std::string &name, sol::table fields) {
        ComponentType component_type;
        for (const auto &pair : fields) {
            std::string key = pair.first.as<std::string>();
            std::string value = pair.second.as<std::string>();

            if (value == "bool") {
                component_type.fields[key] = { component_type.size, typeid(bool).hash_code() };
                component_type.size += sizeof(bool);
            } else if (value == "float") {
                component_type.fields[key] = { component_type.size, typeid(float).hash_code() };
                component_type.size += sizeof(float);
            } else if (value == "string") {
                component_type.fields[key] = { component_type.size, typeid(std::string).hash_code() };
                component_type.size += sizeof(std::string);
            } else {
                YG_CORE_ERROR("ScriptManager: Unknown type!");
            }
        }
        ComponentManager::register_component(name, component_type);
    });
    s_state.set_function("register_system", [](const std::string &name, sol::function on_update, sol::function on_event) {
        SystemManager::register_system(name, on_update, on_event);
    });

    // load script files
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
}

}  // namespace Yogi
