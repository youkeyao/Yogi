#pragma once

#include "Resources/AssetManager/IAssetSource.h"
#include "Resources/AssetManager/AssetSerializer.h"

namespace Yogi
{

class YG_API AssetManager
{
public:
    template <typename T>
    static Ref<T> AddAsset(Handle<T>&& asset, const std::string& key)
    {
        auto& assetMap = GetAssetMap<T>();
        auto  it       = assetMap.find(key);
        if (it == assetMap.end())
        {
            asset.SetSubCallBack([&assetMap, key]() {
                auto it = assetMap.find(key);
                if (it != assetMap.end() && it->second.GetRefCount() == 1)
                    assetMap.erase(it);
            });
            auto [assetIt, result] = assetMap.emplace(key, std::move(asset));
            return Ref<T>::Create(assetIt->second);
        }
        return Ref<T>::Create(it->second);
    }

    template <typename T>
    static Ref<T> GetAsset(const std::string& key)
    {
        auto& assetMap = GetAssetMap<T>();
        auto  it       = assetMap.find(key);
        if (it != assetMap.end())
        {
            return Ref<T>::Create(it->second);
        }
        // not found
        auto& assetSources = GetAssetSources();
        for (auto sourceIt = assetSources.rbegin(); sourceIt != assetSources.rend(); ++sourceIt)
        {
            std::vector<uint8_t> source = (*sourceIt)->LoadSource(key);
            if (!source.empty())
            {
                return AddAsset(GetAssetSerializer<T>()->Deserialize(source, key), key);
            }
        }
        return nullptr;
    }

    template <typename T, typename SerializerType>
    static void RegisterAssetSerializer()
    {
        auto& serializers        = GetAssetSerializers();
        void (*deleterFn)(void*) = +[](void* p) {
            delete static_cast<SerializerType*>(p);
        };
        serializers[typeid(T)] = { new SerializerType(), VoidDeleter(deleterFn) };
    }

    template <typename T, typename... Args>
    static void PushAssetSource(Args&&... args)
    {
        GetAssetSources().push_back(Handle<T>::Create(std::forward<Args>(args)...));
    }
    static void PopAssetSource() { GetAssetSources().pop_back(); }

    static void Clear()
    {
        GetAssetSources().clear();
        GetAssetSerializers().clear();
        auto& maps = GetAssetMaps();
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

    static std::vector<Handle<IAssetSource>>& GetAssetSources()
    {
        static std::vector<Handle<IAssetSource>> sources;
        return sources;
    }

    static std::unordered_map<std::type_index, std::unique_ptr<void, VoidDeleter>>& GetAssetSerializers()
    {
        static std::unordered_map<std::type_index, std::unique_ptr<void, VoidDeleter>> serializers;
        return serializers;
    }

    static std::unordered_map<std::type_index, MapInfo>& GetAssetMaps()
    {
        static std::unordered_map<std::type_index, MapInfo> maps;
        return maps;
    }

    template <typename T>
    static AssetSerializer<T>* GetAssetSerializer()
    {
        auto& serializers = GetAssetSerializers();
        auto  it          = serializers.find(typeid(T));
        if (it != serializers.end())
        {
            return static_cast<AssetSerializer<T>*>(it->second.get());
        }
        return nullptr;
    }

    template <typename T>
    static std::unordered_map<std::string, Handle<T>>& GetAssetMap()
    {
        auto& maps = GetAssetMaps();
        auto  it   = maps.find(typeid(T));
        if (it == maps.end())
        {
            auto* newMap             = new std::unordered_map<std::string, Handle<T>>();
            void (*deleterFn)(void*) = +[](void* p) {
                delete static_cast<std::unordered_map<std::string, Handle<T>>*>(p);
            };
            void (*cleanupFn)(void*) = +[](void* p) {
                auto map = static_cast<std::unordered_map<std::string, Handle<T>>*>(p);
                for (auto& [key, handle] : *map)
                {
                    handle.Cleanup();
                }
            };
            maps[typeid(T)] = MapInfo{ { newMap, VoidDeleter(deleterFn) }, VoidDeleter(cleanupFn) };
            return *newMap;
        }
        return *static_cast<std::unordered_map<std::string, Handle<T>>*>(it->second.Map.get());
    }
};

} // namespace Yogi
