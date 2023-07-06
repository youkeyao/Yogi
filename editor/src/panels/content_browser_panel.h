#pragma once

#include <engine.h>

namespace Yogi {

    class ContentBrowserPanel
    {
    public:
        ContentBrowserPanel(const std::string& dir);
        ~ContentBrowserPanel() = default;

        const std::string get_base_dir() { return m_base_directory.string(); }

        void on_imgui_render();
    private:
        std::filesystem::path m_base_directory;
        std::filesystem::path m_current_directory;
    };

}