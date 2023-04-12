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

}