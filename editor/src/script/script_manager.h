#pragma once

#include <engine.h>
#include <sol/sol.hpp>

namespace Yogi {

class ScriptManager
{
public:
    static void init();
    static void init_project(const std::string &dir_path);
    static void clear();

private:
    static void init_usertype();

private:
    static sol::state *s_state;

    static std::unordered_map<size_t, sol::object (*)(void *)>               s_cast_lua_object_funcs;
    static std::unordered_map<size_t, void (*)(void *, const sol::object &)> s_set_lua_object_funcs;
    static std::unordered_map<std::string, std::pair<size_t, size_t>>        s_basic_fields;
};

}  // namespace Yogi
