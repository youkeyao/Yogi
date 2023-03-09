#pragma once

#include "runtime/core/window.h"

namespace Yogi {

    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

		virtual void init() = 0;

		static Scope<GraphicsContext> create(Window* window);
    };

}