#pragma once

#include "runtime/core/timestep.h"
#include "runtime/events/event.h"

namespace Yogi {

class Layer
{
public:
    Layer(const std::string &name = "Layer") : m_debug_name(name) {}
    virtual ~Layer() = default;

    virtual void on_attach() = 0;
    virtual void on_detach() = 0;
    virtual void on_update(Timestep ts) = 0;
    virtual void on_event(Event &e) = 0;

    inline const std::string &get_name() const { return m_debug_name; }

protected:
    std::string m_debug_name;
};

}  // namespace Yogi