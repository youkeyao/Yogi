#pragma once

#include "runtime/renderer/pipeline.h"

namespace Yogi {

    class PipelineManager
    {
    public:
        static void init();
        static void clear();

        static void add_material(const std::string& key, const Ref<Pipeline>& pipeline);
        static const Ref<Pipeline>& get_pipeline(const std::string& name);
    private:
        static std::map<std::string, Ref<Pipeline>> s_pipelines;
    };

}