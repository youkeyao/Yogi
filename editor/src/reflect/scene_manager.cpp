#include "reflect/scene_manager.h"

#include "reflect/component_manager.h"
#include "reflect/system_manager.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace Yogi {

bool SceneManager::is_scene(std::string json)
{
    rapidjson::Document document;
    document.Parse(json.c_str());
    return !document.HasParseError() && document.IsObject();
}

std::string SceneManager::serialize_scene(const Ref<Scene> &scene)
{
    rapidjson::StringBuffer                    buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();

    writer.Key("entities");
    writer.StartArray();
    scene->each_entity([&writer](Entity entity) {
        writer.StartObject();
        writer.Key("id");
        writer.Int((uint32_t)entity);
        writer.Key("components");
        writer.StartObject();
        entity.each_component([&writer](entt::id_type id_type, void *component) {
            std::string   component_name = ComponentManager::get_component_name(id_type);
            ComponentType type = ComponentManager::get_component_type(component_name);
            writer.Key(component_name.c_str());
            writer.StartObject();
            for (auto [key, value] : type.fields) {
                writer.Key(key.c_str());
                if (value.type_hash == typeid(bool).hash_code()) {
                    writer.Bool(*((bool *)((uint8_t *)component + value.offset)));
                } else if (value.type_hash == typeid(std::string).hash_code()) {
                    writer.String(((std::string *)((uint8_t *)component + value.offset))->c_str());
                } else if (value.type_hash == typeid(float).hash_code()) {
                    writer.Double(*(float *)((uint8_t *)component + value.offset));
                } else if (value.type_hash == typeid(glm::vec2).hash_code()) {
                    writer.StartArray();
                    writer.Double(((glm::vec2 *)((uint8_t *)component + value.offset))->x);
                    writer.Double(((glm::vec2 *)((uint8_t *)component + value.offset))->y);
                    writer.EndArray();
                } else if (value.type_hash == typeid(glm::vec3).hash_code()) {
                    writer.StartArray();
                    writer.Double(((glm::vec3 *)((uint8_t *)component + value.offset))->x);
                    writer.Double(((glm::vec3 *)((uint8_t *)component + value.offset))->y);
                    writer.Double(((glm::vec3 *)((uint8_t *)component + value.offset))->z);
                    writer.EndArray();
                } else if (value.type_hash == typeid(glm::vec4).hash_code()) {
                    writer.StartArray();
                    writer.Double(((glm::vec4 *)((uint8_t *)component + value.offset))->x);
                    writer.Double(((glm::vec4 *)((uint8_t *)component + value.offset))->y);
                    writer.Double(((glm::vec4 *)((uint8_t *)component + value.offset))->z);
                    writer.Double(((glm::vec4 *)((uint8_t *)component + value.offset))->w);
                    writer.EndArray();
                } else if (value.type_hash == typeid(Color).hash_code()) {
                    writer.StartArray();
                    writer.Double(((glm::vec4 *)((uint8_t *)component + value.offset))->x);
                    writer.Double(((glm::vec4 *)((uint8_t *)component + value.offset))->y);
                    writer.Double(((glm::vec4 *)((uint8_t *)component + value.offset))->z);
                    writer.Double(((glm::vec4 *)((uint8_t *)component + value.offset))->w);
                    writer.EndArray();
                } else if (value.type_hash == typeid(Transform).hash_code()) {
                    glm::mat4 &transform = *(glm::mat4 *)((uint8_t *)component + value.offset);
                    writer.StartArray();
                    for (int i = 0; i < 4; i++) {
                        for (int j = 0; j < 4; j++) {
                            writer.Double(transform[i][j]);
                        }
                    }
                    writer.EndArray();
                } else if (value.type_hash == typeid(Entity).hash_code()) {
                    writer.Int((uint32_t) * (Entity *)((uint8_t *)component + value.offset));
                } else if (value.type_hash == typeid(Ref<Mesh>).hash_code()) {
                    Ref<Mesh> &mesh = *(Ref<Mesh> *)((uint8_t *)component + value.offset);
                    writer.String(mesh->name.c_str());
                } else if (value.type_hash == typeid(Ref<Material>).hash_code()) {
                    Ref<Material> &material = *(Ref<Material> *)((uint8_t *)component + value.offset);
                    writer.String(material->get_name().c_str());
                } else if (value.type_hash == typeid(Ref<RenderTexture>).hash_code()) {
                    Ref<RenderTexture> &texture = *(Ref<RenderTexture> *)((uint8_t *)component + value.offset);
                    writer.String(texture ? (texture->get_name()).c_str() : "");
                } else if (value.type_hash == typeid(ColliderType).hash_code()) {
                    ColliderType &type = *(ColliderType *)((uint8_t *)component + value.offset);
                    writer.Int((int)type);
                } else {
                    writer.Bool(false);
                }
            }
            writer.EndObject();
        });
        writer.EndObject();
        writer.EndObject();
    });
    writer.EndArray();

    writer.Key("systems");
    writer.StartArray();
    scene->each_system([&writer](std::string system_name) { writer.String(system_name.c_str()); });
    writer.EndArray();

    writer.EndObject();
    return std::string(buffer.GetString());
}

Ref<Scene> SceneManager::deserialize_scene(std::string json)
{
    Ref<Scene> scene = CreateRef<Scene>();

    rapidjson::Document document;
    document.Parse(json.c_str());

    const rapidjson::Value &entities = document["entities"];
    for (auto &entity_object : entities.GetArray()) {
        uint32_t                id = entity_object["id"].GetInt();
        Entity                  entity = scene->create_entity(id);
        const rapidjson::Value &components = entity_object["components"];
        for (auto &component_object : components.GetObject()) {
            std::string             component_name = component_object.name.GetString();
            const rapidjson::Value &component_fields_value = component_object.value;
            ComponentType           component_type = ComponentManager::get_component_type(component_name);
            void                   *component = ComponentManager::add_component(entity, component_name);
            for (auto [key, value] : component_type.fields) {
                if (value.type_hash == typeid(bool).hash_code()) {
                    bool &v = *(bool *)((uint8_t *)component + value.offset);
                    v = component_fields_value[key.c_str()].GetBool();
                } else if (value.type_hash == typeid(std::string).hash_code()) {
                    std::string &v = *(std::string *)((uint8_t *)component + value.offset);
                    v = component_fields_value[key.c_str()].GetString();
                } else if (value.type_hash == typeid(float).hash_code()) {
                    float &v = *(float *)((uint8_t *)component + value.offset);
                    v = component_fields_value[key.c_str()].GetFloat();
                } else if (value.type_hash == typeid(glm::vec2).hash_code()) {
                    glm::vec2 &vec2 = *(glm::vec2 *)((uint8_t *)component + value.offset);
                    vec2.x = component_fields_value[key.c_str()][0].GetFloat();
                    vec2.y = component_fields_value[key.c_str()][1].GetFloat();
                } else if (value.type_hash == typeid(glm::vec3).hash_code()) {
                    glm::vec3 &vec3 = *(glm::vec3 *)((uint8_t *)component + value.offset);
                    vec3.x = component_fields_value[key.c_str()][0].GetFloat();
                    vec3.y = component_fields_value[key.c_str()][1].GetFloat();
                    vec3.z = component_fields_value[key.c_str()][2].GetFloat();
                } else if (value.type_hash == typeid(glm::vec4).hash_code()) {
                    glm::vec4 &vec4 = *(glm::vec4 *)((uint8_t *)component + value.offset);
                    vec4.x = component_fields_value[key.c_str()][0].GetFloat();
                    vec4.y = component_fields_value[key.c_str()][1].GetFloat();
                    vec4.z = component_fields_value[key.c_str()][2].GetFloat();
                    vec4.w = component_fields_value[key.c_str()][3].GetFloat();
                } else if (value.type_hash == typeid(Color).hash_code()) {
                    glm::vec4 &vec4 = *(glm::vec4 *)((uint8_t *)component + value.offset);
                    vec4.x = component_fields_value[key.c_str()][0].GetFloat();
                    vec4.y = component_fields_value[key.c_str()][1].GetFloat();
                    vec4.z = component_fields_value[key.c_str()][2].GetFloat();
                    vec4.w = component_fields_value[key.c_str()][3].GetFloat();
                } else if (value.type_hash == typeid(Transform).hash_code()) {
                    glm::mat4 &transform = *(glm::mat4 *)((uint8_t *)component + value.offset);
                    for (int i = 0; i < 4; i++) {
                        for (int j = 0; j < 4; j++) {
                            transform[i][j] = component_fields_value[key.c_str()][i * 4 + j].GetFloat();
                        }
                    }
                } else if (value.type_hash == typeid(Entity).hash_code()) {
                    Entity &target = *(Entity *)((uint8_t *)component + value.offset);
                    int32_t handle = component_fields_value[key.c_str()].GetInt();
                    target = handle >= 0 ? scene->get_entity(handle) : Entity{};
                } else if (value.type_hash == typeid(Ref<Mesh>).hash_code()) {
                    Ref<Mesh> &mesh = *(Ref<Mesh> *)((uint8_t *)component + value.offset);
                    mesh = MeshManager::get_mesh(component_fields_value[key.c_str()].GetString());
                } else if (value.type_hash == typeid(Ref<Material>).hash_code()) {
                    Ref<Material> &material = *(Ref<Material> *)((uint8_t *)component + value.offset);
                    material = MaterialManager::get_material(component_fields_value[key.c_str()].GetString());
                } else if (value.type_hash == typeid(Ref<RenderTexture>).hash_code()) {
                    Ref<RenderTexture> &texture = *(Ref<RenderTexture> *)((uint8_t *)component + value.offset);
                    std::string         texture_name = component_fields_value[key.c_str()].GetString();
                    texture = texture_name == "" ? nullptr : TextureManager::get_render_texture(texture_name);
                } else if (value.type_hash == typeid(ColliderType).hash_code()) {
                    ColliderType &type = *(ColliderType *)((uint8_t *)component + value.offset);
                    int32_t       type_id = component_fields_value[key.c_str()].GetInt();
                    type = (ColliderType)type_id;
                }
            }
        }
    }

    const rapidjson::Value &systems = document["systems"];
    for (auto &system : systems.GetArray()) {
        SystemManager::add_system(scene, system.GetString());
    }

    return scene;
}

}  // namespace Yogi