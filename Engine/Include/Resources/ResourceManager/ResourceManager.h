#pragma once

#include "Resources/ResourceManager/ResourceHash.h"

namespace Yogi
{

class YG_API ResourceManager
{
public:
    template <typename T>
    static Ref<T> AddResource(Handle<T>&& resource, uint64_t key)
    {
        auto& resourceMap = GetResourceMap<T>();
        auto  it          = resourceMap.find(key);
        if (it == resourceMap.end())
        {
            resource.SetSubCallBack([&resourceMap, key]() {
                auto it = resourceMap.find(key);
                if (it != resourceMap.end() && it->second.GetRefCount() == 1)
                    resourceMap.erase(it);
            });
            auto [resourceIt, result] = resourceMap.emplace(key, std::move(resource));
            return Ref<T>::Create(resourceIt->second);
        }
        return Ref<T>::Create(it->second);
    }

    template <typename T, typename... Args>
    static Ref<T> GetResource(Args&&... args)
    {
        auto&    resourceMap = GetResourceMap<T>();
        uint64_t key         = HashArgs(std::forward<Args>(args)...);
        auto     it          = resourceMap.find(key);
        if (it != resourceMap.end())
        {
            return Ref<T>::Create(it->second);
        }
        // not found
        return AddResource(T::Create(std::forward<Args>(args)...), key);
    }

    static void Clear() { s_resourceMaps.clear(); }

protected:
    struct VoidDeleter
    {
        using Fn = void (*)(void*);
        Fn fn;
        VoidDeleter() noexcept : fn(nullptr) {}
        explicit VoidDeleter(Fn f) noexcept : fn(f) {}
        void operator()(void* p) const noexcept
        {
            if (fn)
                fn(p);
        }
    };

    using Any = std::unique_ptr<void, VoidDeleter>;

    template <typename T>
    static std::unordered_map<uint64_t, Handle<T>>& GetResourceMap()
    {
        auto it = s_resourceMaps.find(typeid(T));
        if (it == s_resourceMaps.end())
        {
            auto* newMap             = new std::unordered_map<uint64_t, Handle<T>>();
            void (*deleterFn)(void*) = +[](void* p) {
                delete static_cast<std::unordered_map<uint64_t, Handle<T>>*>(p);
            };
            s_resourceMaps[typeid(T)] = { newMap, VoidDeleter(deleterFn) };
            return *newMap;
        }
        return *static_cast<std::unordered_map<uint64_t, Handle<T>>*>((it->second).get());
    }

private:
    static std::unordered_map<std::type_index, Any> s_resourceMaps;
};

} // namespace Yogi
