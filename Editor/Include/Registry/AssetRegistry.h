#pragma once

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

private:
    static std::vector<RegisterKeyFunc>                           s_registerKeyFuncs;
    static std::unordered_map<uint32_t, std::vector<std::string>> s_keyMaps;
};

} // namespace Yogi
