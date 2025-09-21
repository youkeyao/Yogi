#include "Reflect/ComponentManager.h"

namespace Yogi
{

std::unordered_map<uint32_t, ComponentManager::ComponentInfo> ComponentManager::s_componentInfos;

void ComponentManager::Init()
{
    RegisterComponent<TagComponent>({ "Tag" });
    RegisterComponent<TransformComponent>({ "Parent", "Transform" });
    RegisterComponent<MeshRendererComponent>({ "Mesh", "Material", "CastShadow" });
    RegisterComponent<CameraComponent>({ "Fov", "ZoomLevel", "IsOrtho" });
}

void ComponentManager::Clear() { s_componentInfos.clear(); }

template <typename T>
void ComponentManager::RegisterComponent(const std::vector<std::string>& fieldNames)
{
    ComponentInfo componentInfo;
    componentInfo.Type.Name = GetTypeName<T>();
    if (s_componentInfos.find(componentInfo.Type.TypeHash) != s_componentInfos.end())
        return;

    T obj;
    boost::pfr::for_each_field(obj, [&](auto& field, std::size_t i) {
        Field fieldInfo;
        fieldInfo.Name     = fieldNames[i];
        fieldInfo.Offset   = (std::size_t)&field - (std::size_t)&obj;
        fieldInfo.TypeHash = GetTypeHash<decltype(field)>();
        componentInfo.Type.Fields.push_back(fieldInfo);
    });
    componentInfo.Type.TypeHash = GetTypeHash<T>();
    componentInfo.Type.Size     = sizeof(T);

    componentInfo.AddFunc = [](Entity& entity) {
        return (void*)&entity.AddComponent<T>();
    };
    componentInfo.RemoveFunc = [](Entity& entity) {
        entity.RemoveComponent<T>();
    };

    s_componentInfos.insert({ componentInfo.Type.TypeHash, componentInfo });
}

void ComponentManager::EachComponentType(std::function<void(ComponentType&)>&& func)
{
    for (auto& [hash, info] : s_componentInfos)
        func(info.Type);
}

void ComponentManager::AddComponent(Entity& entity, uint32_t typeHash) { s_componentInfos[typeHash].AddFunc(entity); }

void ComponentManager::RemoveComponent(Entity& entity, uint32_t typeHash)
{
    s_componentInfos[typeHash].RemoveFunc(entity);
}

ComponentType& ComponentManager::GetComponentType(uint32_t typeHash)
{
    if (s_componentInfos.find(typeHash) != s_componentInfos.end())
        return s_componentInfos[typeHash].Type;
    YG_CORE_ASSERT(false, "Get invalid component type!");
    static ComponentType nullType;
    return nullType;
}

} // namespace Yogi