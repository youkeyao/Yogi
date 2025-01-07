#pragma once

#include "runtime/core/timestep.h"
#include "runtime/events/event.h"

namespace Yogi {

class SystemBase
{
public:
    virtual ~SystemBase() = default;
    virtual void on_update(Timestep ts, Scene &scene) = 0;
    virtual void on_event(Event &e, Scene &scene) = 0;
};

}  // namespace Yogi