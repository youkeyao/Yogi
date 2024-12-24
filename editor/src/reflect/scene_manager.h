#pragma once

#include <engine.h>

namespace Yogi {

class SceneManager
{
public:
    static std::string serialize_scene(const Ref<Scene> &scene);
    static Ref<Scene>  deserialize_scene(std::string json);
    static bool        is_scene(std::string json);
};

}  // namespace Yogi