#pragma once

namespace Yogi
{

template <typename T>
class AssetSerializer
{
public:
    virtual ~AssetSerializer() = default;

    virtual Scope<T> Deserialize(const std::vector<uint8_t>& binary, const std::string& key) = 0;
    virtual std::vector<uint8_t> Serialize(const Scope<T>& asset, const std::string& key)    = 0;
};

} // namespace Yogi
