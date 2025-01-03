#pragma once

#include <sol/sol.hpp>

namespace Yogi {

class ScriptManager
{
public:
    static void init(const std::string &dir_path);
    static void clear();

private:
    static sol::state                         s_state;
    static std::map<std::string, std::string> s_scripts;
};

}  // namespace Yogi
