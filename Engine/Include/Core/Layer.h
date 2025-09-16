#pragma once

#include "Core/Timestep.h"
#include "Events/Event.h"

namespace Yogi
{

class YG_API Layer
{
public:
    Layer(const std::string& name = "Layer") : m_debugName(name) {}
    virtual ~Layer() = default;

    virtual void OnUpdate(Timestep ts) = 0;
    virtual void OnEvent(Event& e)     = 0;

    inline const std::string& GetName() const { return m_debugName; }

protected:
    std::string m_debugName;
};

} // namespace Yogi