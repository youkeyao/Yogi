#pragma once

#include "Core/Timestep.h"
#include "Events/Event.h"
#include "Scene/ComponentBase.h"

namespace Yogi
{

class World;
class SystemBase
{
public:
    virtual ~SystemBase() = default;

    virtual void OnUpdate(Timestep ts, World& world) = 0;
    virtual void OnEvent(Event& e, World& world)     = 0;
};

} // namespace Yogi