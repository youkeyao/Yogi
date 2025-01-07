#pragma once

#include <engine.h>
#include <sol/sol.hpp>

namespace Yogi {

class ScriptManager
{
protected:
    static void init_usertype();

public:
    static void init();
    static void init_project(const std::string &dir_path);
    static void clear();

private:
    static sol::state *s_state;
};

}  // namespace Yogi
