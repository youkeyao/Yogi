#pragma once

#include "Resources/IAssetSource.h"
#include "Resources/AssetSerializer.h"

namespace Yogi
{

// Forward declare
class AssetManager;
template <typename T>
class YG_API AssetHandle
{
    friend class AssetManager;

public:
    AssetHandle() : m_resource(nullptr), m_key(nullptr) {}
    AssetHandle(const AssetHandle& other);
    AssetHandle(AssetHandle&& other) noexcept : m_resource(other.m_resource), m_key(other.m_key)
    {
        other.m_resource = nullptr;
        other.m_key      = nullptr;
    }

    AssetHandle& operator=(const AssetHandle& other);
    AssetHandle& operator=(AssetHandle&& other) noexcept;

    ~AssetHandle();

    View<T> operator->() const { return m_resource; }
    View<T> Get() const { return m_resource; }

    void Release();

protected:
    AssetHandle(const View<T>& resource, const View<std::string> key);

private:
    View<T>           m_resource = nullptr;
    View<std::string> m_key      = nullptr;
};

// ---------------------------------------------------------------------------------------------
class YG_API AssetManager
{
public:
    template <typename T>
    static AssetHandle<T> AddAsset(Scope<T> asset, const std::string& key)
    {
        auto& assetMap = GetAssetMap<T>();
        auto  it       = assetMap.find(key);
        if (it == assetMap.end())
        {
            auto [assetIt, result] = assetMap.emplace(key, std::make_pair(std::move(asset), 0));
            return AssetHandle<T>(CreateView(assetIt->second.first), CreateView(assetIt->first));
        }
        return AssetHandle<T>(nullptr, nullptr);
    }

    template <typename T>
    static AssetHandle<T> GetAsset(const std::string& key)
    {
        auto& assetMap = GetAssetMap<T>();
        auto  it       = assetMap.find(key);
        if (it != assetMap.end())
        {
            return AssetHandle<T>(CreateView(it->second.first), CreateView(it->first));
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
        return AssetHandle<T>(nullptr, nullptr);
    }

    template <typename T>
    static void ReleaseAsset(const View<std::string>& key)
    {
        GetAssetMap<T>().erase(*key);
    }

    template <typename T>
    static void AddRef(const View<std::string>& key)
    {
        if (!key) return;
        auto& assetMap = GetAssetMap<T>();
        auto  it       = assetMap.find(*key);
        if (it != assetMap.end())
        {
            ++(it->second.second);
        }
    }

    template <typename T>
    static void SubRef(const View<std::string>& key)
    {
        if (!key) return;
        auto& assetMap = GetAssetMap<T>();
        auto  it       = assetMap.find(*key);
        if (it != assetMap.end())
        {
            if (--(it->second.second) == 0)
            {
                ReleaseAsset<T>(key);
            }
        }
    }

    template <typename T>
    static void RegisterAssetSerializer(Scope<AssetSerializer<T>> serializer)
    {
        GetAssetSerializer<T>() = std::move(serializer);
    }

    static void PushAssetSource(Scope<IAssetSource> source) { GetAssetSources().push_back(std::move(source)); }
    static void PopAssetSource() { GetAssetSources().pop_back(); }

protected:
    static std::vector<Scope<IAssetSource>>& GetAssetSources()
    {
        static std::vector<Scope<IAssetSource>> sources;
        return sources;
    }

    template <typename T>
    static Scope<AssetSerializer<T>>& GetAssetSerializer()
    {
        static Scope<AssetSerializer<T>> serializer = nullptr;
        return serializer;
    }

    template <typename T>
    static std::unordered_map<std::string, std::pair<Scope<T>, uint32_t>>& GetAssetMap()
    {
        static std::unordered_map<std::string, std::pair<Scope<T>, uint32_t>> map;
        return map;
    }
};

// Implementations for AssetHandle
template <typename T>
AssetHandle<T>::AssetHandle(const View<T>& resource, const View<std::string> key) :
    m_resource(resource),
    m_key(key)
{
    AssetManager::AddRef<T>(m_key);
}

template <typename T>
AssetHandle<T>::AssetHandle(const AssetHandle& other) : m_resource(other.m_resource), m_key(other.m_key)
{
    AssetManager::AddRef<T>(m_key);
}

template <typename T>
AssetHandle<T>& AssetHandle<T>::operator=(const AssetHandle<T>& other)
{
    if (this != &other)
    {
        Release();
        m_resource = other.m_resource;
        m_key      = other.m_key;
        AssetManager::AddRef<T>(m_key);
    }
    return *this;
}

template <typename T>
AssetHandle<T>& AssetHandle<T>::operator=(AssetHandle&& other) noexcept
{
    if (this != &other)
    {
        Release();
        m_resource       = other.m_resource;
        m_key            = other.m_key;
        other.m_resource = nullptr;
        other.m_key      = nullptr;
    }
    return *this;
}

template <typename T>
AssetHandle<T>::~AssetHandle()
{
    Release();
}

template <typename T>
void AssetHandle<T>::Release()
{
    AssetManager::SubRef<T>(m_key);
    m_resource = nullptr;
    m_key      = nullptr;
}

} // namespace Yogi
