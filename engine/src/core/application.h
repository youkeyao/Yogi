#pragma once

namespace hazel {

    class Application
    {
    public:
        Application();
        virtual ~Application();
        void run();
    };

    extern Application* create_application();

}