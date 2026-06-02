#pragma once

#include "Core/Singleton.h"
#include "Resources/AssetManager/IAssetSource.h"
#include "Resources/AssetManager/Serializer/AssetSerializer.h"

namespace Yogi
{

class YG_API AssetManager : public Singleton<AssetManager>
{
    friend class Singleton<AssetManager>;

public:
    static size_t CollectUnusedAssetsAll(size_t maxCollectPerType = std::numeric_limits<size_t>::max())
    {
        auto&  self           = Get();
        size_t totalCollected = 0;
        for (auto& [_, entry] : self.m_assetMaps)
        {
            if (entry.collectFn)
            {
                totalCollected += entry.collectFn(entry.data.get(), maxCollectPerType);
            }
        }
        return totalCollected;
    }

    struct AssetStats
    {
        size_t assetCount  = 0;
        size_t keyMapCount = 0;
    };

    struct AssetStatsAll
    {
        size_t totalAssets = 0;
        size_t totalTypes  = 0;
    };

    template <typename T>
    static AssetStats GetAssetStats()
    {
        AssetStats stats;
        auto&      map    = GetAssetMap<T>();
        stats.assetCount  = map.size();
        stats.keyMapCount = GetAssetKeyMap<T>().size();
        return stats;
    }

    static AssetStatsAll GetAssetStatsAll()
    {
        auto&         self = Get();
        AssetStatsAll stats;
        stats.totalAssets = 0;
        for (const auto& [typeIdx, entry] : self.m_assetMaps)
        {
            (void)typeIdx;
            if (entry.data)
            {
                auto* map = static_cast<std::unordered_map<std::string, Owner<void>>*>(entry.data.get());
                stats.totalAssets += map->size();
            }
        }
        stats.totalTypes = self.m_assetMaps.size();
        return stats;
    }

    template <typename T>
    static size_t CollectUnusedAssets(size_t maxCollect = std::numeric_limits<size_t>::max())
    {
        auto&  assetMap  = GetAssetMap<T>();
        size_t collected = 0;
        auto   it        = assetMap.begin();
        while (it != assetMap.end() && collected < maxCollect)
        {
            if (WRef<T>::Create(it->second).Expired())
            {
                GetAssetKeyMap<T>().erase(GetAssetIdentity(it->second));
                it = assetMap.erase(it);
                ++collected;
            }
            else
            {
                ++it;
            }
        }

        RemoveAssetKeyMapIfEmpty<T>();
        return collected;
    }

    template <typename T>
    static WRef<T> SetAsset(Owner<T>&& asset, const std::string& key)
    {
        auto& assetMap = GetAssetMap<T>();
        auto& keyMap   = GetAssetKeyMap<T>();

        auto it = assetMap.find(key);
        if (it != assetMap.end())
        {
            if (it->second)
            {
                keyMap.erase(GetAssetIdentity(it->second));
            }
            it->second = std::move(asset);
        }
        else
        {
            it = assetMap.emplace(key, std::move(asset)).first;
        }

        if (it->second)
        {
            keyMap[GetAssetIdentity(it->second)] = key;
        }
        return WRef<T>::Create(it->second);
    }

    template <typename T>
    static WRef<T> AcquireAsset(const std::string& key)
    {
        auto& assetMap = GetAssetMap<T>();
        auto  it       = assetMap.find(key);
        if (it != assetMap.end())
        {
            return WRef<T>::Create(it->second);
        }

        auto* serializer = GetAssetSerializer<T>();
        if (!serializer)
        {
            YG_CORE_ERROR("AssetSerializer is not registered");
            return nullptr;
        }

        auto& sources = Get().m_sources;
        for (auto sourceIt = sources.rbegin(); sourceIt != sources.rend(); ++sourceIt)
        {
            auto source = (*sourceIt)->LoadSource(key);
            if (source)
            {
                return SetAsset(serializer->Deserialize(*source, key), key);
            }
        }
        return nullptr;
    }

    template <typename T>
    static void SaveAsset(const WRef<T>& asset, const std::string& key, int sourceIndex = 0)
    {
        auto& sources = Get().m_sources;
        if (sourceIndex < 0 || sourceIndex >= static_cast<int>(sources.size()))
        {
            YG_CORE_ERROR("Invalid AssetSource index: {0}", sourceIndex);
            return;
        }

        auto* serializer = GetAssetSerializer<T>();
        if (!serializer)
        {
            YG_CORE_ERROR("AssetSerializer is not registered");
            return;
        }

        sources[sourceIndex]->SaveSource(key, serializer->Serialize(asset, key));
    }

    template <typename T>
    static std::string GetAssetKey(const WRef<T>& asset)
    {
        return FindAssetKeyOrEmpty<T>(GetAssetIdentity(asset));
    }

    template <typename T>
    static std::string GetAssetKey(const T* asset)
    {
        return FindAssetKeyOrEmpty<T>(static_cast<const void*>(asset));
    }

    template <typename T, typename SerializerType>
    static void RegisterAssetSerializer()
    {
        void (*deleterFn)(void*) = +[](void* p) {
            delete static_cast<SerializerType*>(p);
        };
        Get().m_serializers[typeid(T)] = { new SerializerType(), VoidDeleter(deleterFn) };
    }

    template <typename T, typename... Args>
    static void PushAssetSource(Args&&... args)
    {
        Get().m_sources.push_back(Owner<T>::Create(std::forward<Args>(args)...));
    }

    static void PopAssetSource()
    {
        auto& sources = Get().m_sources;
        if (sources.empty())
        {
            YG_CORE_ERROR("No AssetSource to pop");
            return;
        }
        sources.pop_back();
    }

    static WRef<IAssetSource> AcquireAssetSource(int sourceIndex = 0)
    {
        auto& sources = Get().m_sources;
        if (sourceIndex < 0 || sourceIndex >= static_cast<int>(sources.size()))
        {
            YG_CORE_ERROR("Invalid AssetSource index: {0}", sourceIndex);
            return nullptr;
        }
        return WRef<IAssetSource>::Create(sources[sourceIndex]);
    }

    template <typename T>
    static std::unordered_map<std::string, Owner<T>>& GetAssetMap()
    {
        auto& maps = Get().m_assetMaps;
        auto  it   = maps.find(typeid(T));
        if (it == maps.end())
        {
            auto* newMap             = new std::unordered_map<std::string, Owner<T>>();
            void (*deleterFn)(void*) = +[](void* p) {
                delete static_cast<std::unordered_map<std::string, Owner<T>>*>(p);
            };
            maps[typeid(T)] = MapEntry{ Any{ newMap, VoidDeleter(deleterFn) }, &CollectUnusedAssetsImpl<T> };
            return *newMap;
        }
        return *static_cast<std::unordered_map<std::string, Owner<T>>*>(it->second.data.get());
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

    struct MapEntry
    {
        Any         data;
        CollectorFn collectFn = nullptr;
    };

    template <typename T>
    static size_t CollectUnusedAssetsImpl(void* rawMap, size_t maxCollect)
    {
        auto&  assetMap  = *static_cast<std::unordered_map<std::string, Owner<T>>*>(rawMap);
        size_t collected = 0;
        auto   it        = assetMap.begin();
        while (it != assetMap.end() && collected < maxCollect)
        {
            if (WRef<T>::Create(it->second).Expired())
            {
                GetAssetKeyMap<T>().erase(GetAssetIdentity(it->second));
                it = assetMap.erase(it);
                ++collected;
            }
            else
            {
                ++it;
            }
        }

        RemoveAssetKeyMapIfEmpty<T>();
        return collected;
    }

    template <typename T>
    static AssetSerializer<T>* GetAssetSerializer()
    {
        auto& serializers = Get().m_serializers;
        auto  it          = serializers.find(typeid(T));
        if (it != serializers.end())
        {
            return static_cast<AssetSerializer<T>*>(it->second.get());
        }
        return nullptr;
    }

    template <typename T>
    static const void* GetAssetIdentity(const Owner<T>& asset)
    {
        return asset.Get();
    }

    template <typename T>
    static const void* GetAssetIdentity(const WRef<T>& asset)
    {
        return asset.Get();
    }

    template <typename T>
    static std::unordered_map<const void*, std::string>& GetAssetKeyMap()
    {
        auto& keyMaps = Get().m_assetKeyMaps;
        auto  it      = keyMaps.find(typeid(T));
        if (it == keyMaps.end())
        {
            auto* newMap             = new std::unordered_map<const void*, std::string>();
            void (*deleterFn)(void*) = +[](void* p) {
                delete static_cast<std::unordered_map<const void*, std::string>*>(p);
            };
            keyMaps[typeid(T)] = Any{ newMap, VoidDeleter(deleterFn) };
            return *newMap;
        }
        return *static_cast<std::unordered_map<const void*, std::string>*>(it->second.get());
    }

    template <typename T>
    static std::string FindAssetKeyOrEmpty(const void* identity)
    {
        if (!identity)
        {
            return "";
        }

        auto& keyMaps = Get().m_assetKeyMaps;
        auto  it      = keyMaps.find(typeid(T));
        if (it == keyMaps.end())
        {
            return "";
        }

        auto& keyMap = *static_cast<std::unordered_map<const void*, std::string>*>(it->second.get());
        auto  keyIt  = keyMap.find(identity);
        if (keyIt != keyMap.end())
        {
            return keyIt->second;
        }
        return "";
    }

    template <typename T>
    static void RebuildAssetKeyMap()
    {
        auto& assetMap = GetAssetMap<T>();
        auto& keyMap   = GetAssetKeyMap<T>();
        keyMap.clear();
        for (const auto& [key, owner] : assetMap)
        {
            if (owner)
            {
                keyMap[GetAssetIdentity(owner)] = key;
            }
        }
    }

    template <typename T>
    static void RemoveAssetKeyMapIfEmpty()
    {
        auto& keyMaps = Get().m_assetKeyMaps;
        auto  it      = keyMaps.find(typeid(T));
        if (it == keyMaps.end())
        {
            return;
        }

        auto& keyMap = *static_cast<std::unordered_map<const void*, std::string>*>(it->second.get());
        if (keyMap.empty())
        {
            keyMaps.erase(it);
        }
    }

private:
    AssetManager()  = default;
    ~AssetManager() = default;

    static AssetManager* s_instance; // defined in .cpp, exported via YG_API

    std::vector<Owner<IAssetSource>>              m_sources;
    std::unordered_map<std::type_index, Any>      m_serializers;
    std::unordered_map<std::type_index, MapEntry> m_assetMaps;
    std::unordered_map<std::type_index, Any>      m_assetKeyMaps;
};

} // namespace Yogi
