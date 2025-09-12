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
            resource.SetSubCallBack([&]() {
                auto it = resourceMap.find(key);
                if (it != resourceMap.end() && it->second.GetRefCount() == 1)
                    resourceMap.erase(key);
            });
            auto [resourceIt, result] = resourceMap.emplace(key, std::move(resource));
            return Ref<T>::Create(resourceIt->second);
        }
        return nullptr;
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

protected:
    template <typename T>
    static std::unordered_map<uint64_t, Handle<T>>& GetResourceMap()
    {
        static std::unordered_map<uint64_t, Handle<T>> map;
        return map;
    }
};

} // namespace Yogi
