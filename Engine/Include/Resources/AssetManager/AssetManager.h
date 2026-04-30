#pragma once

#include "Resources/AssetManager/IAssetSource.h"
#include "Resources/AssetManager/Serializer/AssetSerializer.h"

namespace Yogi
{

class YG_API AssetManager
{
public:
    static size_t CollectUnusedAssetsAll(size_t maxCollectPerType = std::numeric_limits<size_t>::max())
    {
        size_t totalCollected = 0;
        for (auto& [_, entry] : s_assetMaps)
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
        AssetStatsAll stats;
        stats.totalAssets = 0;
        for (const auto& [typeIdx, entry] : s_assetMaps)
        {
            (void)typeIdx;
            if (entry.data)
            {
                auto* map = static_cast<std::unordered_map<std::string, Owner<void>>*>(entry.data.get());
                stats.totalAssets += map->size();
            }
        }
        stats.totalTypes = s_assetMaps.size();
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

        for (auto sourceIt = s_sources.rbegin(); sourceIt != s_sources.rend(); ++sourceIt)
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
        if (sourceIndex < 0 || sourceIndex >= static_cast<int>(s_sources.size()))
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

        s_sources[sourceIndex]->SaveSource(key, serializer->Serialize(asset, key));
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
        s_serializers[typeid(T)] = { new SerializerType(), VoidDeleter(deleterFn) };
    }

    template <typename T, typename... Args>
    static void PushAssetSource(Args&&... args)
    {
        s_sources.push_back(Owner<T>::Create(std::forward<Args>(args)...));
    }

    static void PopAssetSource()
    {
        if (s_sources.empty())
        {
            YG_CORE_ERROR("No AssetSource to pop");
            return;
        }
        s_sources.pop_back();
    }

    static WRef<IAssetSource> AcquireAssetSource(int sourceIndex = 0)
    {
        if (sourceIndex < 0 || sourceIndex >= static_cast<int>(s_sources.size()))
        {
            YG_CORE_ERROR("Invalid AssetSource index: {0}", sourceIndex);
            return nullptr;
        }
        return WRef<IAssetSource>::Create(s_sources[sourceIndex]);
    }

    template <typename T>
    static std::unordered_map<std::string, Owner<T>>& GetAssetMap()
    {
        auto it = s_assetMaps.find(typeid(T));
        if (it == s_assetMaps.end())
        {
            auto* newMap             = new std::unordered_map<std::string, Owner<T>>();
            void (*deleterFn)(void*) = +[](void* p) {
                delete static_cast<std::unordered_map<std::string, Owner<T>>*>(p);
            };
            s_assetMaps[typeid(T)] = MapEntry{ Any{ newMap, VoidDeleter(deleterFn) }, &CollectUnusedAssetsImpl<T> };
            return *newMap;
        }
        return *static_cast<std::unordered_map<std::string, Owner<T>>*>(it->second.data.get());
    }

    static void Clear()
    {
        s_sources.clear();
        s_serializers.clear();
        s_assetMaps.clear();
        s_assetKeyMaps.clear();
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
        auto it = s_serializers.find(typeid(T));
        if (it != s_serializers.end())
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
        auto it = s_assetKeyMaps.find(typeid(T));
        if (it == s_assetKeyMaps.end())
        {
            auto* newMap             = new std::unordered_map<const void*, std::string>();
            void (*deleterFn)(void*) = +[](void* p) {
                delete static_cast<std::unordered_map<const void*, std::string>*>(p);
            };
            s_assetKeyMaps[typeid(T)] = Any{ newMap, VoidDeleter(deleterFn) };
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

        auto it = s_assetKeyMaps.find(typeid(T));
        if (it == s_assetKeyMaps.end())
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
        auto it = s_assetKeyMaps.find(typeid(T));
        if (it == s_assetKeyMaps.end())
        {
            return;
        }

        auto& keyMap = *static_cast<std::unordered_map<const void*, std::string>*>(it->second.get());
        if (keyMap.empty())
        {
            s_assetKeyMaps.erase(it);
        }
    }

private:
    static std::vector<Owner<IAssetSource>>              s_sources;
    static std::unordered_map<std::type_index, Any>      s_serializers;
    static std::unordered_map<std::type_index, MapEntry> s_assetMaps;
    static std::unordered_map<std::type_index, Any>      s_assetKeyMaps;
};

} // namespace Yogi
