#pragma once

#include <engine.h>

class LauncherLayer : public Yogi::Layer
{
public:
    LauncherLayer(const std::string &file_path = "Launcher");
    ~LauncherLayer() = default;

    void on_attach() override;
    void on_detach() override;
    void on_update(Yogi::Timestep ts) override;
    void on_event(Yogi::Event &event) override;

private:
    Yogi::Ref<Yogi::Scene> m_scene;
    std::string m_file_path;
};