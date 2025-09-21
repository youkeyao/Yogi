#pragma once

namespace Yogi
{

class IAssetSource
{
public:
    virtual ~IAssetSource() = default;

    virtual std::vector<uint8_t> LoadSource(const std::string& key)                                   = 0;
    virtual void                 SaveSource(const std::string& key, const std::vector<uint8_t>& data) = 0;
};

} // namespace Yogi