#pragma once

#include "Scene/Entity.h"
#include "Math/Transform.h"

namespace Yogi
{

struct TagComponent
{
    std::string Tag = "";
};

struct TransformComponent
{
    Entity    Parent = Entity::Null();
    Transform Transform;
};

} // namespace Yogi
