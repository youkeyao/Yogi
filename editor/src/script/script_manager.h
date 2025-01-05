#pragma once

#include <sol/sol.hpp>

namespace Yogi {

class ScriptManager
{
protected:
    static void init_usertype();

public:
    static void init(const std::string &dir_path);

    static void create_component(const std::string &component_name, const std::vector<std::string> &field_names);

private:
    static sol::state s_state;
};

}  // namespace Yogi
