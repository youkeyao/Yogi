#include "Resources/AssetManager/FileSystemSource.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/AssetManager/MeshSerializer.h"

namespace Yogi
{

std::vector<uint8_t> LoadBinaryFile(const std::string& filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
    {
        YG_CORE_ERROR("Failed to open file: {0}", filepath);
        return {};
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    file.close();

    return data;
}

// ----------------------------------------------------------------------------------------------

FileSystemSource::FileSystemSource(const std::string& rootDir) : m_rootDir(rootDir)
{
    AssetManager::RegisterAssetSerializer<Mesh, MeshSerializer>();
}

std::vector<uint8_t> FileSystemSource::LoadSource(const std::string& key)
{
    size_t sepPos = key.find("::");

    std::string filepath = "";
    if (sepPos == std::string::npos)
    {
        filepath = m_rootDir + key;
    }
    else
    {
        filepath = m_rootDir + key.substr(0, sepPos);
    }
    return LoadBinaryFile(filepath);
}

} // namespace Yogi
