#pragma once

#include <engine.h>

namespace Yogi {

class SystemManager
{
private:
    typedef void (*SystemFunc)(Ref<Scene>, const std::string &);
    typedef std::function<void(Timestep, Scene *)> UpdateFunc;
    typedef std::function<void(Event &, Scene *)>  EventFunc;

    class RuntimeSystem : public SystemBase
    {
    public:
        RuntimeSystem(UpdateFunc update_function, EventFunc event_function)
            : m_update_function(update_function), m_event_function(event_function)
        {
        }
        void on_update(Timestep ts, Scene *scene) override { m_update_function(ts, scene); }
        void on_event(Event &e, Scene *scene) override { m_event_function(e, scene); }

    private:
        UpdateFunc m_update_function;
        EventFunc  m_event_function;
    };

public:
    static void init();
    static void clear();
    static void add_system(Ref<Scene> scene, const std::string &system_name);
    static void remove_system(Ref<Scene> scene, const std::string &system_name);
    static void each_system_type(std::function<void(std::string)> func);

    template <typename Type>
    static void register_system()
    {
        std::string system_name = get_type_name<Type>();
        s_add_system_funcs[system_name] = [](Ref<Scene> scene, const std::string &system_name) {
            scene->add_system<Type>();
        };
        s_remove_system_funcs[system_name] = [](Ref<Scene> scene, const std::string &system_name) {
            scene->remove_system<Type>();
        };
    }
    template <typename F1, typename F2>
    static void register_system(const std::string &system_name, F1 update_function, F2 event_function)
    {
        s_runtime_systems[system_name] = { update_function, event_function };
        s_add_system_funcs[system_name] = [](Ref<Scene> scene, const std::string &system_name) {
            auto [update_function, event_function] = s_runtime_systems[system_name];
            scene->add_runtime_system(system_name, CreateRef<RuntimeSystem>(update_function, event_function));
        };
        s_remove_system_funcs[system_name] = [](Ref<Scene> scene, const std::string &system_name) {
            scene->remove_runtime_system(system_name);
        };
    }

private:
    static std::unordered_map<std::string, SystemFunc> s_add_system_funcs;
    static std::unordered_map<std::string, SystemFunc> s_remove_system_funcs;

    static std::unordered_map<std::string, std::pair<UpdateFunc, EventFunc>> s_runtime_systems;
};

}  // namespace Yogi