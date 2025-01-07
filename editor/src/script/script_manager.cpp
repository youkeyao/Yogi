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

sol::state *ScriptManager::s_state = nullptr;

void ScriptManager::init_usertype()
{
    s_state->new_usertype<Timestep>(
        "Timestep", sol::constructors<Timestep(float)>(),  // constructor
        "get_seconds", &Timestep::get_seconds,             // get seconds func
        "get_miliseconds", &Timestep::get_miliseconds      // get miliseconds func
    );
    s_state->new_usertype<Event>(
        "Event", sol::no_constructor,              // constructor
        "get_event_type", &Event::get_event_type,  // get event type
        "m_handled", &Event::m_handled             // check if event is handled
    );
    s_state->new_usertype<RuntimeComponent>(
        "RuntimeComponent", sol::no_constructor,  // constructor
        sol::meta_function::index,
        [](RuntimeComponent &comp, const std::string &field_name) -> sol::object {
            void *data = comp.get_data();
            auto &type = comp.get_type();
            auto &it = type.fields.find(field_name);
            YG_ASSERT(it != type.fields.end(), "RuntimeComponent get unknown field name: {0}!", field_name);
            const Field &field = it->second;
            void        *field_ptr = (uint8_t *)data + field.offset;

            if (field.type_hash == typeid(bool).hash_code()) {
                return sol::make_object(*s_state, *static_cast<bool *>(field_ptr));
            } else if (field.type_hash == typeid(int).hash_code()) {
                return sol::make_object(*s_state, *static_cast<int *>(field_ptr));
            } else if (field.type_hash == typeid(float).hash_code()) {
                return sol::make_object(*s_state, *static_cast<float *>(field_ptr));
            } else if (field.type_hash == typeid(std::string).hash_code()) {
                return sol::make_object(*s_state, *static_cast<std::string *>(field_ptr));
            } else if (field.type_hash == typeid(glm::vec2).hash_code()) {
                return sol::make_object(*s_state, *static_cast<glm::vec2 *>(field_ptr));
            } else if (field.type_hash == typeid(glm::vec3).hash_code()) {
                return sol::make_object(*s_state, *static_cast<glm::vec3 *>(field_ptr));
            } else if (field.type_hash == typeid(glm::vec4).hash_code()) {
                return sol::make_object(*s_state, *static_cast<glm::vec4 *>(field_ptr));
            } else if (field.type_hash == typeid(Color).hash_code()) {
                return sol::make_object(*s_state, *static_cast<Color *>(field_ptr));
            } else if (field.type_hash == typeid(Transform).hash_code()) {
                return sol::make_object(*s_state, *static_cast<Transform *>(field_ptr));
            } else if (field.type_hash == typeid(Entity).hash_code()) {
                return sol::make_object(*s_state, *static_cast<Entity *>(field_ptr));
            } else if (field.type_hash == typeid(Ref<Mesh>).hash_code()) {
                return sol::make_object(*s_state, *static_cast<Ref<Mesh> *>(field_ptr));
            } else if (field.type_hash == typeid(Ref<Material>).hash_code()) {
                return sol::make_object(*s_state, *static_cast<Ref<Material> *>(field_ptr));
            } else if (field.type_hash == typeid(Ref<RenderTexture>).hash_code()) {
                return sol::make_object(*s_state, *static_cast<Ref<RenderTexture> *>(field_ptr));
            }
            return sol::nil;
        },  // get field
        sol::meta_function::new_index,
        [](RuntimeComponent &comp, const std::string &field_name, sol::object value) {
            YG_PROFILE_SCOPE(("ScriptManager::set_field" + field_name).c_str());
            void *data = comp.get_data();
            auto &type = comp.get_type();
            auto &it = type.fields.find(field_name);
            YG_ASSERT(it != type.fields.end(), "RuntimeComponent set unknown field name: {0}!", field_name);
            const Field &field = it->second;
            void        *field_ptr = (uint8_t *)data + field.offset;

            if (field.type_hash == typeid(bool).hash_code()) {
                *static_cast<bool *>(field_ptr) = value.as<bool>();
            } else if (field.type_hash == typeid(int).hash_code()) {
                *static_cast<int *>(field_ptr) = value.as<int>();
            } else if (field.type_hash == typeid(float).hash_code()) {
                *static_cast<float *>(field_ptr) = value.as<float>();
            } else if (field.type_hash == typeid(std::string).hash_code()) {
                *static_cast<std::string *>(field_ptr) = value.as<std::string>();
            } else if (field.type_hash == typeid(glm::vec2).hash_code()) {
                *static_cast<glm::vec2 *>(field_ptr) = value.as<glm::vec2>();
            } else if (field.type_hash == typeid(glm::vec3).hash_code()) {
                *static_cast<glm::vec3 *>(field_ptr) = value.as<glm::vec3>();
            } else if (field.type_hash == typeid(glm::vec4).hash_code()) {
                *static_cast<glm::vec4 *>(field_ptr) = value.as<glm::vec4>();
            } else if (field.type_hash == typeid(Color).hash_code()) {
                *static_cast<Color *>(field_ptr) = value.as<Color>();
            } else if (field.type_hash == typeid(Transform).hash_code()) {
                *static_cast<Transform *>(field_ptr) = value.as<Transform>();
            } else if (field.type_hash == typeid(Entity).hash_code()) {
                *static_cast<Entity *>(field_ptr) = value.as<Entity>();
            } else if (field.type_hash == typeid(Ref<Mesh>).hash_code()) {
                *static_cast<Ref<Mesh> *>(field_ptr) = value.as<Ref<Mesh>>();
            } else if (field.type_hash == typeid(Ref<Material>).hash_code()) {
                *static_cast<Ref<Material> *>(field_ptr) = value.as<Ref<Material>>();
            } else if (field.type_hash == typeid(Ref<RenderTexture>).hash_code()) {
                *static_cast<Ref<RenderTexture> *>(field_ptr) = value.as<Ref<RenderTexture>>();
            }
        }  // set field
    );
    s_state->new_usertype<Entity>(
        "Entity", sol::no_constructor,  // constructor
        "add_component",
        [](Entity &entity, const std::string &component_name) -> RuntimeComponent {
            YG_PROFILE_SCOPE("add_component");
            void *component = ComponentManager::add_component(entity, component_name);
            return RuntimeComponent{ component, &ComponentManager::get_component_type(component_name) };
        },  // add component
        "get_component",
        [](Entity &entity, const std::string &component_name) -> RuntimeComponent {
            const auto &component_type = ComponentManager::get_component_type(component_name);
            return RuntimeComponent{ entity.get_runtime_component(component_type.type_id), &component_type };
        },  // get component
        "remove_component",
        [](Entity &entity, const std::string &component_name) {
            const auto &component_type = ComponentManager::get_component_type(component_name);
            entity.remove_runtime_component(component_type.type_id);
        }  // remove component
    );
    s_state->new_usertype<Scene>(
        "Scene", sol::no_constructor,                    // constructor
        "create_entity", &Scene::create_entity,          // create entity
        "get_entity", &Scene::get_entity,                // get entity
        "delete_entity", &Scene::delete_entity,          // delete entity
        "each_entity", &Scene::each_entity,              // each entity
        "each_system", &Scene::each_system,              // each system
        "remove_system", &Scene::remove_runtime_system,  // remove system
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
    s_state->new_usertype<glm::vec2>(
        "vec2", sol::call_constructor, sol::constructors<glm::vec2(), glm::vec2(float, float)>(),  // constructor
        "x", &glm::vec2::x,                                                                        // x
        "y", &glm::vec2::y                                                                         // y
    );
    s_state->new_usertype<glm::vec3>(
        "vec3", sol::call_constructor, sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),  // constructor
        "x", &glm::vec3::x,                                                                               // x
        "y", &glm::vec3::y,                                                                               // y
        "z", &glm::vec3::z                                                                                // z
    );
    s_state->new_usertype<glm::vec4>(
        "vec4", sol::call_constructor, sol::constructors<glm::vec4(), glm::vec4(float, float, float, float)>(),  // constructor
        "x", &glm::vec4::x,                                                                                      // x
        "y", &glm::vec4::y,                                                                                      // y
        "z", &glm::vec4::z,                                                                                      // z
        "w", &glm::vec4::w                                                                                       // w
    );
    s_state->new_usertype<Color>(
        "Color", sol::call_constructor, sol::constructors<Color(), Color(const glm::vec4 &)>(),  // constructor
        "r",
        sol::property(
            [](Color &color) -> float { return ((glm::vec4)color).r; },
            [](Color &color, float v) {
                glm::vec4 &vec = (glm::vec4)color;
                color = glm::vec4{ v, vec.g, vec.b, vec.a };
            }),  // r
        "g",
        sol::property(
            [](Color &color) -> float { return ((glm::vec4)color).g; },
            [](Color &color, float v) {
                glm::vec4 &vec = (glm::vec4)color;
                color = glm::vec4{ vec.r, v, vec.b, vec.a };
            }),  // g
        "b",
        sol::property(
            [](Color &color) -> float { return ((glm::vec4)color).b; },
            [](Color &color, float v) {
                glm::vec4 &vec = (glm::vec4)color;
                color = glm::vec4{ vec.r, vec.g, v, vec.a };
            }),  // b
        "a",
        sol::property(
            [](Color &color) -> float { return ((glm::vec4)color).a; },
            [](Color &color, float v) {
                glm::vec4 &vec = (glm::vec4)color;
                color = glm::vec4{ vec.r, vec.g, vec.b, v };
            })  // a
    );
    s_state->new_usertype<Transform>(
        "Transform", sol::no_constructor,  // constructor
        "translate", sol::factories([](const glm::vec3 &translation) {
            return Transform{ glm::translate(glm::mat4(1.0f), translation) };
        }),  // translate
        "rotate", sol::factories([](float angle, const glm::vec3 &axis) {
            return Transform{ glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis) };
        }),  // rotate
        "scale",
        sol::factories([](const glm::vec3 &scale) { return Transform{ glm::scale(glm::mat4(1.0f), scale) }; }),  // scale
        "lookat", sol::factories([](const glm::vec3 &position, const glm::vec3 &target, const glm::vec3 &up) {
            return Transform{ glm::lookAt(position, target, up) };
        }),                                                                               // lookat
        "inverse", [](Transform &a) -> Transform { return glm::inverse((glm::mat4)a); },  // inverse
        sol::meta_function::multiplication,
        [](Transform &a, Transform &b) -> Transform {
            return a * b;
        }  // multiplication
    );
    s_state->new_usertype<Mesh>(
        "Mesh", sol::no_constructor,  // constructor
        "get_name",
        [](Mesh &mesh) -> const std::string & {
            return mesh.name;
        }  // get name
    );
    s_state->new_usertype<MeshManager>(
        "MeshManager", sol::no_constructor,  // constructor
        "get_mesh", &MeshManager::get_mesh   // get mesh
    );
    s_state->new_usertype<Material>(
        "Material", sol::no_constructor,  // constructor
        "get_name", &Material::get_name   // get name
    );
    s_state->new_usertype<MaterialManager>(
        "MaterialManager", sol::no_constructor,         // constructor
        "get_material", &MaterialManager::get_material  // get material
    );
}

void ScriptManager::init()
{
    if (s_state == nullptr) {
        s_state = new sol::state();
    }
    init_usertype();
    lua_register(*s_state, "print", lua_print);
    // register component and system
    s_state->set_function("register_component", [](const std::string &name, sol::table fields) {
        ComponentType component_type;
        for (const auto &pair : fields) {
            std::string key = pair.first.as<std::string>();
            std::string value = pair.second.as<std::string>();

            if (value == "bool") {
                component_type.fields[key] = { component_type.size, typeid(bool).hash_code() };
                component_type.size += sizeof(bool);
            } else if (value == "int") {
                component_type.fields[key] = { component_type.size, typeid(int).hash_code() };
                component_type.size += sizeof(int);
            } else if (value == "float") {
                component_type.fields[key] = { component_type.size, typeid(float).hash_code() };
                component_type.size += sizeof(float);
            } else if (value == "string") {
                component_type.fields[key] = { component_type.size, typeid(std::string).hash_code() };
                component_type.size += sizeof(std::string);
            } else if (value == "vec2") {
                component_type.fields[key] = { component_type.size, typeid(glm::vec2).hash_code() };
                component_type.size += sizeof(glm::vec2);
            } else if (value == "vec3") {
                component_type.fields[key] = { component_type.size, typeid(glm::vec3).hash_code() };
                component_type.size += sizeof(glm::vec3);
            } else if (value == "vec4") {
                component_type.fields[key] = { component_type.size, typeid(glm::vec4).hash_code() };
                component_type.size += sizeof(glm::vec4);
            } else if (value == "Color") {
                component_type.fields[key] = { component_type.size, typeid(Color).hash_code() };
                component_type.size += sizeof(Color);
            } else if (value == "Transform") {
                component_type.fields[key] = { component_type.size, typeid(Transform).hash_code() };
                component_type.size += sizeof(Transform);
            } else if (value == "Entity") {
                component_type.fields[key] = { component_type.size, typeid(Entity).hash_code() };
                component_type.size += sizeof(Entity);
            } else if (value == "Mesh") {
                component_type.fields[key] = { component_type.size, typeid(Ref<Mesh>).hash_code() };
                component_type.size += sizeof(Ref<Mesh>);
            } else if (value == "Material") {
                component_type.fields[key] = { component_type.size, typeid(Ref<Material>).hash_code() };
                component_type.size += sizeof(Ref<Material>);
            } else if (value == "RenderTexture") {
                component_type.fields[key] = { component_type.size, typeid(Ref<RenderTexture>).hash_code() };
                component_type.size += sizeof(Ref<RenderTexture>);
            } else {
                YG_CORE_ERROR("ScriptManager: Unknown type!");
            }
        }
        ComponentManager::register_component(name, component_type);
    });
    s_state->set_function("register_system", [](const std::string &name, sol::function on_update, sol::function on_event) {
        SystemManager::register_system(
            name,
            [on_update, name](Timestep ts, Scene &scene) {
#ifdef YG_SCRIPT_NO_SAFE
                sol::function func = on_update;
                func(ts, scene);
#else
                sol::safe_function func = on_update;
                auto         &result = func(ts, scene);
                if (!result.valid()) {
                    sol::error err = result;
                    YG_ERROR("{0} update error: {1}", name, err.what());
                }
#endif
            },
            [on_event, name](Event &event, Scene &scene) {
#ifdef YG_SCRIPT_NO_SAFE
                sol::function func = on_event;
                func(event, scene);
#else
                sol::safe_function func = on_event;
                auto         &result = func(event, scene);
                if (!result.valid()) {
                    sol::error err = result;
                    YG_ERROR("{0} event error: {1}", name, err.what());
                }
#endif
            });
    });
}

void ScriptManager::init_project(const std::string &dir_path)
{
    // load script files
    if (std::filesystem::is_directory(dir_path)) {
        for (auto &directory_entry : std::filesystem::directory_iterator(dir_path)) {
            const auto &path = directory_entry.path();
            std::string filename = path.stem().string();
            std::string extension = path.extension().string();

            if (directory_entry.is_directory()) {
                init_project(path.string());
            } else if (extension == ".lua") {
#ifdef YG_SCRIPT_NO_SAFE
                s_state->script_file(path.string());
#else
                auto &result = s_state->script_file(path.string());
                if (!result.valid()) {
                    sol::error err = result;
                    YG_CORE_ERROR("ScriptManager error in {0}: {1}", path.string(), err.what());
                }
#endif
            }
        }
    }
}

void ScriptManager::clear()
{
    delete s_state;
    s_state = nullptr;
}

}  // namespace Yogi
