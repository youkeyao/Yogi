#pragma once

#include "Resources/IAssetSource.h"

namespace Yogi
{

class YG_API FileSystemSource : public IAssetSource
{
public:
    FileSystemSource(const std::string& rootDir);

    std::vector<uint8_t> LoadSource(const std::string& key) override;

private:
    std::string m_rootDir;
};

} // namespace Yogi
