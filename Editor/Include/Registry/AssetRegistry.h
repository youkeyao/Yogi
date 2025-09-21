#pragma once

#include <Yogi.h>

namespace Yogi
{

class AssetRegistry
{
    typedef void (*RegisterKeyFunc)(const std::filesystem::path&                            path,
                                    std::unordered_map<uint32_t, std::vector<std::string>>& keyMaps);

public:
    static void Init();
    static void Scan(const std::string& rootDir);
    static void Clear();

    static void Register(RegisterKeyFunc&& func);

    template <typename T>
    static const std::vector<std::string>& GetKeys()
    {
        return s_keyMaps[GetTypeHash<T>()];
    }

    template <typename T>
    static std::string GetKey(const Ref<T>& asset)
    {
        auto& assetMap = AssetManager::GetAssetMap<T>();
        for (const auto& [key, value] : assetMap)
        {
            if (value == asset)
            {
                return key;
            }
        }
        return "";
    }

private:
    static std::vector<RegisterKeyFunc>                           s_registerKeyFuncs;
    static std::unordered_map<uint32_t, std::vector<std::string>> s_keyMaps;
};

} // namespace Yogi
