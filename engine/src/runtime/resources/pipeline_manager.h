#pragma once

#include "runtime/renderer/pipeline.h"

namespace Yogi {

    class PipelineManager
    {
    public:
        static void init();
        static void clear();
        static const Ref<Pipeline>& get_pipeline(const std::string& name) { return s_pipelines[name]; }
    private:
        static std::map<std::string, Ref<Pipeline>> s_pipelines;
    };

}