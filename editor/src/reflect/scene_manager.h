#pragma once

#include <engine.h>

namespace Yogi {

    class SceneManager
    {
    public:
        static void save_scene(Ref<Scene> scene, std::string filepath);
        static Ref<Scene> load_scene(std::string filepath);
    };

}