#pragma once

#include "core/timestep.h"
#include "events/event.h"

namespace hazel {

    class Layer
    {
    public:
        Layer(const std::string& name = "Layer");
        virtual ~Layer();

        virtual void on_attach() {}
        virtual void on_detach() {}
        virtual void on_update(TimeStep ts) {}
        virtual void on_event(Event& e) {}
        virtual void on_imgui_render() {}

        inline const std::string& get_name() const { return m_debug_name; }
    protected:
        std::string m_debug_name;
    };

}