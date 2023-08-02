#pragma once

#include "runtime/renderer/pipeline.h"

namespace Yogi {

    class PipelineManager
    {
    public:
        static void init();
        static void clear();

        static void add_pipeline(const std::string& key, const Ref<Pipeline>& pipeline);
        static const Ref<Pipeline>& get_pipeline(const std::string& name);

        static void each_pipeline(std::function<void(const Ref<Pipeline>&)> func);
    private:
        static std::map<std::string, Ref<Pipeline>> s_pipelines;
    };

}