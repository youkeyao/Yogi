#pragma once

#include "runtime/core/timestep.h"
#include "runtime/events/event.h"

namespace Yogi {

    class Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer() = default;

        virtual void on_attach() {}
        virtual void on_detach() {}
        virtual void on_update(Timestep ts) {}
        virtual void on_event(Event& e) {}

        inline const std::string& get_name() const { return m_debug_name; }
    protected:
        std::string m_debug_name;
    };

}