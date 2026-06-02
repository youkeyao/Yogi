#pragma once

#include "Core/Singleton.h"
#include "Resources/ResourceManager/ResourceHash.h"

namespace Yogi
{

class YG_API ResourceManager : public Singleton<ResourceManager>
{
    friend class Singleton<ResourceManager>;

public:
    static size_t CollectUnusedResourcesAll(size_t maxCollectPerType = std::numeric_limits<size_t>::max())
    {
        auto&  self           = Get();
        size_t totalCollected = 0;
        for (auto& [_, entry] : self.m_resourceMaps)
        {
            if (entry.collectFn)
            {
                totalCollected += entry.collectFn(entry.data.get(), maxCollectPerType);
            }
        }
        for (auto& [_, entry] : self.m_resourceLists)
        {
            if (entry.collectFn)
            {
                totalCollected += entry.collectFn(entry.data.get(), maxCollectPerType);
            }
        }
        return totalCollected;
    }

    template <typename T>
    static size_t CollectUnusedResources(size_t maxCollect = std::numeric_limits<size_t>::max())
    {
        size_t collected = CollectUnusedMappedResources<T>(maxCollect);
        if (collected >= maxCollect)
        {
            return collected;
        }
        return collected + CollectUnusedCreatedResources<T>(maxCollect - collected);
    }

    template <typename T>
    static size_t CollectUnusedMappedResources(size_t maxCollect = std::numeric_limits<size_t>::max())
    {
        return CollectUnusedMappedResourcesImpl<T>(GetResourceMap<T>(), maxCollect);
    }

    template <typename T>
    static size_t CollectUnusedCreatedResources(size_t maxCollect = std::numeric_limits<size_t>::max())
    {
        return CollectUnusedCreatedResourcesImpl<T>(GetResourceList<T>(), maxCollect);
    }

    template <typename T>
    static WRef<T> AddResource(Owner<T>&& resource, uint64_t key)
    {
        auto& resourceMap = GetResourceMap<T>();
        auto  it          = resourceMap.find(key);
        if (it == resourceMap.end())
        {
            auto [resourceIt, result] = resourceMap.emplace(key, std::move(resource));
            return WRef<T>::Create(resourceIt->second);
        }
        return WRef<T>::Create(it->second);
    }

    template <typename T>
    static WRef<T> AddResource(Owner<T>&& resource)
    {
        auto& resourceList = GetResourceList<T>();
        resourceList.push_back(std::move(resource));
        return WRef<T>::Create(resourceList.back());
    }

    template <typename T, typename... Args>
    static WRef<T> AcquireSharedResource(Args&&... args)
    {
        auto&    resourceMap = GetResourceMap<T>();
        uint64_t key         = HashArgs(std::forward<Args>(args)...);
        auto     it          = resourceMap.find(key);
        if (it != resourceMap.end())
        {
            return WRef<T>::Create(it->second);
        }
        return AddResource(Owner<T>::Create(std::forward<Args>(args)...), key);
    }

    template <typename T, typename... Args>
    static WRef<T> CreateResource(Args&&... args)
    {
        return AddResource(Owner<T>::Create(std::forward<Args>(args)...));
    }

    struct ResourceStats
    {
        size_t sharedCount = 0;
        size_t uniqueCount = 0;
        size_t totalCount  = 0;
    };

    struct ResourceStatsAll
    {
        size_t totalShared    = 0;
        size_t totalUnique    = 0;
        size_t totalResources = 0;
    };

    template <typename T>
    static ResourceStats GetResourceStats()
    {
        ResourceStats stats;
        {
            auto& map         = GetResourceMap<T>();
            stats.sharedCount = map.size();
        }
        {
            auto& list        = GetResourceList<T>();
            stats.uniqueCount = list.size();
        }
        stats.totalCount = stats.sharedCount + stats.uniqueCount;
        return stats;
    }

    static ResourceStatsAll GetResourceStatsAll()
    {
        auto&            self = Get();
        ResourceStatsAll stats;
        stats.totalShared    = self.m_resourceMaps.size();
        stats.totalUnique    = self.m_resourceLists.size();
        stats.totalResources = stats.totalShared + stats.totalUnique;
        return stats;
    }

protected:
    using CollectorFn = size_t (*)(void*, size_t);

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

    struct Entry
    {
        Any         data;
        CollectorFn collectFn = nullptr;
    };

    template <typename T>
    static size_t CollectUnusedMappedResourcesImpl(std::unordered_map<uint64_t, Owner<T>>& resourceMap,
                                                   size_t                                  maxCollect)
    {
        size_t collected = 0;
        auto   it        = resourceMap.begin();
        while (it != resourceMap.end() && collected < maxCollect)
        {
            if (WRef<T>::Create(it->second).Expired())
            {
                it = resourceMap.erase(it);
                ++collected;
            }
            else
            {
                ++it;
            }
        }
        return collected;
    }

    template <typename T>
    static size_t CollectUnusedCreatedResourcesImpl(std::vector<Owner<T>>& resourceList, size_t maxCollect)
    {
        size_t collected = 0;
        auto   it        = resourceList.begin();
        while (it != resourceList.end() && collected < maxCollect)
        {
            if (WRef<T>::Create(*it).Expired())
            {
                it = resourceList.erase(it);
                ++collected;
            }
            else
            {
                ++it;
            }
        }
        return collected;
    }

    template <typename T>
    static size_t CollectUnusedMappedResourcesThunk(void* rawMap, size_t maxCollect)
    {
        auto& resourceMap = *static_cast<std::unordered_map<uint64_t, Owner<T>>*>(rawMap);
        return CollectUnusedMappedResourcesImpl<T>(resourceMap, maxCollect);
    }

    template <typename T>
    static size_t CollectUnusedCreatedResourcesThunk(void* rawList, size_t maxCollect)
    {
        auto& resourceList = *static_cast<std::vector<Owner<T>>*>(rawList);
        return CollectUnusedCreatedResourcesImpl<T>(resourceList, maxCollect);
    }

    template <typename T>
    static std::unordered_map<uint64_t, Owner<T>>& GetResourceMap()
    {
        auto& maps = Get().m_resourceMaps;
        auto  it   = maps.find(typeid(T));
        if (it == maps.end())
        {
            auto* newMap             = new std::unordered_map<uint64_t, Owner<T>>();
            void (*deleterFn)(void*) = +[](void* p) {
                delete static_cast<std::unordered_map<uint64_t, Owner<T>>*>(p);
            };
            maps[typeid(T)] = Entry{ Any{ newMap, VoidDeleter(deleterFn) }, &CollectUnusedMappedResourcesThunk<T> };
            return *newMap;
        }
        return *static_cast<std::unordered_map<uint64_t, Owner<T>>*>(it->second.data.get());
    }

    template <typename T>
    static std::vector<Owner<T>>& GetResourceList()
    {
        auto& lists = Get().m_resourceLists;
        auto  it    = lists.find(typeid(T));
        if (it == lists.end())
        {
            auto* newList            = new std::vector<Owner<T>>();
            void (*deleterFn)(void*) = +[](void* p) {
                delete static_cast<std::vector<Owner<T>>*>(p);
            };
            lists[typeid(T)] = Entry{ Any{ newList, VoidDeleter(deleterFn) }, &CollectUnusedCreatedResourcesThunk<T> };
            return *newList;
        }
        return *static_cast<std::vector<Owner<T>>*>(it->second.data.get());
    }

private:
    ResourceManager()  = default;
    ~ResourceManager() = default;

    static ResourceManager* s_instance; // defined in .cpp, exported via YG_API

    std::unordered_map<std::type_index, Entry> m_resourceMaps;
    std::unordered_map<std::type_index, Entry> m_resourceLists;
};

} // namespace Yogi
