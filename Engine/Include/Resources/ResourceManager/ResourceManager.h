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

    static void Clear()
    {
        auto& maps = GetResourceMaps();
        for (auto& [type, mapInfo] : maps)
        {
            mapInfo.CleanupFn(mapInfo.Map.get());
        }
        maps.clear();
    }

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

    struct MapInfo
    {
        std::unique_ptr<void, VoidDeleter> Map;
        VoidDeleter                        CleanupFn;
    };

    static std::unordered_map<std::type_index, MapInfo>& GetResourceMaps()
    {
        static std::unordered_map<std::type_index, MapInfo> maps;
        return maps;
    }

    template <typename T>
    static std::unordered_map<uint64_t, Handle<T>>& GetResourceMap()
    {
        auto& maps = GetResourceMaps();
        auto  it   = maps.find(typeid(T));
        if (it == maps.end())
        {
            auto* newMap             = new std::unordered_map<uint64_t, Handle<T>>();
            void (*deleterFn)(void*) = +[](void* p) {
                delete static_cast<std::unordered_map<uint64_t, Handle<T>>*>(p);
            };
            void (*cleanupFn)(void*) = +[](void* p) {
                auto map = static_cast<std::unordered_map<uint64_t, Handle<T>>*>(p);
                for (auto& [key, handle] : *map)
                {
                    handle.Cleanup();
                }
            };
            maps[typeid(T)] = MapInfo{ { newMap, VoidDeleter(deleterFn) }, VoidDeleter(cleanupFn) };
            return *newMap;
        }
        return *static_cast<std::unordered_map<uint64_t, Handle<T>>*>((it->second.Map).get());
    }
};

} // namespace Yogi
