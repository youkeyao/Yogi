#pragma once

#include "Resources/AssetManager/IAssetSource.h"

namespace Yogi
{

class YG_API FileSystemSource : public IAssetSource
{
public:
    FileSystemSource(const std::string& rootDir);

    std::vector<uint8_t> LoadSource(const std::string& key) override;
    void                 SaveSource(const std::string& key, const std::vector<uint8_t>& data) override;

private:
    std::filesystem::path m_rootDir;
};

} // namespace Yogi
