#pragma once

namespace Yogi
{

class YG_API IAssetSource
{
public:
    virtual ~IAssetSource() = default;
    virtual std::vector<uint8_t> LoadSource(const std::string& key) = 0;
};

} // namespace Yogi