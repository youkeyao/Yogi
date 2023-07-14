#include "runtime/resources/pipeline_manager.h"

namespace Yogi {

    std::map<std::string, Ref<Pipeline>> PipelineManager::s_pipelines{};

    void PipelineManager::init()
    {
        s_pipelines["Flat"] = Pipeline::create("Flat");
    }

    void PipelineManager::clear()
    {
        s_pipelines.clear();
    }

    void PipelineManager::add_material(const std::string& key, const Ref<Pipeline>& pipeline)
    {
        s_pipelines[key] = pipeline;
    }

    const Ref<Pipeline>& PipelineManager::get_pipeline(const std::string& name)
    {
        if (s_pipelines.find(name) != s_pipelines.end())
            return s_pipelines[name];
        else
            return s_pipelines["Flat"];
    }

}