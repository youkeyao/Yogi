#pragma once

#include <engine.h>

namespace Yogi {

    class SceneManager
    {
    public:
        static std::string serialize_scene(Ref<Scene> scene);
        static Ref<Scene> deserialize_scene(std::string json);
    };

}