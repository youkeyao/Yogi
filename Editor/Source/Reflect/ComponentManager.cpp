#include "Reflect/ComponentManager.h"

namespace Yogi
{

std::unordered_map<uint32_t, ComponentType>                         ComponentManager::s_componentTypes;
std::unordered_map<uint32_t, ComponentManager::AddComponentFunc>    ComponentManager::s_addComponentFuncs;
std::unordered_map<uint32_t, ComponentManager::RemoveComponentFunc> ComponentManager::s_removeComponentFuncs;

void ComponentManager::Init()
{
    RegisterComponent<TagComponent>({ "Tag" });
    RegisterComponent<TransformComponent>({ "Parent", "Transform" });
    RegisterComponent<MeshRendererComponent>({ "Mesh", "Material", "CastShadow" });
    RegisterComponent<CameraComponent>({ "Fov", "AspectRatio", "ZoomLevel", "IsOrtho" });
}

void ComponentManager::Clear()
{
    s_componentTypes.clear();
    s_addComponentFuncs.clear();
    s_removeComponentFuncs.clear();
}

template <typename T>
void ComponentManager::RegisterComponent(const std::vector<std::string>& fieldNames)
{
    ComponentType componentType;
    componentType.Name = GetTypeName<T>();
    if (s_componentTypes.find(componentType.TypeHash) != s_componentTypes.end())
        return;

    T obj;
    boost::pfr::for_each_field(obj, [&](auto& field, std::size_t i) {
        Field fieldInfo;
        fieldInfo.Name     = fieldNames[i];
        fieldInfo.Offset   = (std::size_t)&field - (std::size_t)&obj;
        fieldInfo.TypeHash = GetTypeHash<decltype(field)>();
        componentType.Fields.push_back(fieldInfo);
    });
    componentType.TypeHash = GetTypeHash<T>();
    componentType.Size     = sizeof(T);

    s_componentTypes.insert({ componentType.TypeHash, componentType });
    s_addComponentFuncs.insert({ componentType.TypeHash, [](Entity& entity) {
                                    return (void*)&entity.AddComponent<T>();
                                } });
    s_removeComponentFuncs.insert({ componentType.TypeHash, [](Entity& entity) {
                                       entity.RemoveComponent<T>();
                                   } });
}

void ComponentManager::EachComponentType(std::function<void(ComponentType&)> func)
{
    for (auto& [name, type] : s_componentTypes)
        func(type);
}

void ComponentManager::AddComponent(Entity& entity, uint32_t typeHash) { s_addComponentFuncs[typeHash](entity); }

void ComponentManager::RemoveComponent(Entity& entity, uint32_t typeHash) { s_removeComponentFuncs[typeHash](entity); }

ComponentType& ComponentManager::GetComponentType(uint32_t typeHash) { return s_componentTypes[typeHash]; }

} // namespace Yogi