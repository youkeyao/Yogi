#pragma once

#include <Yogi.h>

namespace Yogi
{

class ContentBrowserLayer : public Layer
{
public:
    ContentBrowserLayer();

    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

private:
    std::filesystem::path m_baseDirectory;
    std::filesystem::path m_relativeDirectory;
};

} // namespace Yogi
