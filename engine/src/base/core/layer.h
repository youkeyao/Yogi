#pragma once

#include "base/core/timestep.h"
#include "base/events/event.h"

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
        virtual void on_imgui_render() {}

        inline const std::string& get_name() const { return m_debug_name; }
    protected:
        std::string m_debug_name;
    };

}