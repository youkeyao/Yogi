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
        for (auto sourceIt = s_sources.rbegin(); sourceIt != s_sources.rend(); ++sourceIt)
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
        void (*deleterFn)(void*) = +[](void* p) {
            delete static_cast<SerializerType*>(p);
        };
        s_serializers[typeid(T)] = { new SerializerType(), VoidDeleter(deleterFn) };
    }

    template <typename T, typename... Args>
    static void PushAssetSource(Args&&... args)
    {
        s_sources.push_back(Handle<T>::Create(std::forward<Args>(args)...));
    }
    static void PopAssetSource() { s_sources.pop_back(); }

    static void Clear()
    {
        s_sources.clear();
        s_serializers.clear();
        s_assetMaps.clear();
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

    using Any = std::unique_ptr<void, VoidDeleter>;

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
    static std::unordered_map<std::string, Handle<T>>& GetAssetMap()
    {
        auto it = s_assetMaps.find(typeid(T));
        if (it == s_assetMaps.end())
        {
            auto* newMap             = new std::unordered_map<std::string, Handle<T>>();
            void (*deleterFn)(void*) = +[](void* p) {
                delete static_cast<std::unordered_map<std::string, Handle<T>>*>(p);
            };
            s_assetMaps[typeid(T)] = Any{ newMap, VoidDeleter(deleterFn) };
            return *newMap;
        }
        return *static_cast<std::unordered_map<std::string, Handle<T>>*>(it->second.get());
    }

private:
    static std::vector<Handle<IAssetSource>>        s_sources;
    static std::unordered_map<std::type_index, Any> s_serializers;
    static std::unordered_map<std::type_index, Any> s_assetMaps;
};

} // namespace Yogi
