#include "runtime/resources/pipeline_manager.h"

namespace Yogi {

std::map<std::string, Ref<Pipeline>> PipelineManager::s_pipelines{};

void PipelineManager::init()
{
    std::map<std::string, std::vector<std::string>> pipelines;
    if (std::filesystem::is_directory(YG_SHADER_DIR)) {
        for (auto &directory_entry : std::filesystem::directory_iterator(YG_SHADER_DIR)) {
            const auto &path = directory_entry.path();
            std::string filename = path.stem().string();
            std::string extension = path.extension().string().erase(0, 1);

            if (directory_entry.is_directory()) {
                continue;
            } else {
                pipelines[filename].push_back(extension);
            }
        }
    }
    for (auto &[name, types] : pipelines) {
        add_pipeline(name, Pipeline::create(name, types));
    }
}

void PipelineManager::clear()
{
    s_pipelines.clear();
}

void PipelineManager::add_pipeline(const std::string &key, const Ref<Pipeline> &pipeline)
{
    s_pipelines[key] = pipeline;
}

const Ref<Pipeline> &PipelineManager::get_pipeline(const std::string &name)
{
    if (s_pipelines.find(name) != s_pipelines.end())
        return s_pipelines[name];
    else
        return s_pipelines["Flat"];
}

void PipelineManager::each_pipeline(std::function<void(const Ref<Pipeline> &)> func)
{
    for (auto [key, pipeline] : s_pipelines) {
        func(pipeline);
    }
}

}  // namespace Yogi