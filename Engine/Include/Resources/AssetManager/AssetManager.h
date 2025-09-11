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
            asset.SetSubCallBack([&]() {
                if (asset.GetRefCount() == 1)
                    assetMap.erase(key);
            });
            auto [assetIt, result] = assetMap.emplace(key, std::move(asset));
            return Ref<T>::Create(assetIt->second);
        }
        return nullptr;
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

    template <typename T>
    static void RegisterAssetSerializer(Handle<AssetSerializer<T>>&& serializer)
    {
        GetAssetSerializer<T>() = std::move(serializer);
    }

    static void PushAssetSource(Handle<IAssetSource>&& source) { GetAssetSources().push_back(std::move(source)); }
    static void PopAssetSource() { GetAssetSources().pop_back(); }

protected:
    static std::vector<Handle<IAssetSource>>& GetAssetSources()
    {
        static std::vector<Handle<IAssetSource>> sources;
        return sources;
    }

    template <typename T>
    static Handle<AssetSerializer<T>>& GetAssetSerializer()
    {
        static Handle<AssetSerializer<T>> serializer = nullptr;
        return serializer;
    }

    template <typename T>
    static std::unordered_map<std::string, Handle<T>>& GetAssetMap()
    {
        static std::unordered_map<std::string, Handle<T>> map;
        return map;
    }
};

} // namespace Yogi
