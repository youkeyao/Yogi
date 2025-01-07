#include <engine.h>

#include "runtime/core/entrypoint.h"
#include "launcher_layer.h"

class Launcher : public Yogi::Application
{
public:
    Launcher() { push_layer(new LauncherLayer(LAUNCHER_DIR "/sandbox")); }

    ~Launcher() {}
};

Yogi::Application *Yogi::create_application()
{
    return new Launcher();
}