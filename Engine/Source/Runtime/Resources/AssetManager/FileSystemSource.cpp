#include "Resources/AssetManager/FileSystemSource.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/AssetManager/MeshSerializer.h"
#include "Resources/AssetManager/MaterialSerializer.h"
#include "Resources/AssetManager/TextureSerializer.h"
#include "Resources/AssetManager/ShaderSerializer.h"

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
    AssetManager::RegisterAssetSerializer<Material, MaterialSerializer>();
    AssetManager::RegisterAssetSerializer<ITexture, TextureSerializer>();
    AssetManager::RegisterAssetSerializer<ShaderDesc, ShaderSerializer>();
}

std::vector<uint8_t> FileSystemSource::LoadSource(const std::string& key)
{
    size_t sepPos = key.find("::");

    std::filesystem::path filepath;
    if (sepPos == std::string::npos)
    {
        filepath = m_rootDir / key;
    }
    else
    {
        filepath = m_rootDir / key.substr(0, sepPos);
    }
    return LoadBinaryFile(filepath.string());
}

void FileSystemSource::SaveSource(const std::string& key, const std::vector<uint8_t>& data)
{
    size_t sepPos = key.find("::");

    std::filesystem::path filepath;
    if (sepPos == std::string::npos)
    {
        filepath = m_rootDir / key;
    }
    else
    {
        filepath = m_rootDir / key.substr(0, sepPos);
    }

    std::ofstream file(filepath, std::ios::binary);
    if (!file)
    {
        YG_CORE_ERROR("Failed to open file for writing: {0}", filepath.string());
        return;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
}

} // namespace Yogi
