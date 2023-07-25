#pragma once

#include "runtime/scene/entity.h"
#include "runtime/core/timestep.h"
#include "runtime/events/event.h"
#include "runtime/renderer/material.h"
#include "runtime/renderer/frame_buffer.h"
#include <entt/entity/registry.hpp>

namespace Yogi {

    class Scene
    {
        typedef void(*SystemUpdateFunc)(Timestep, Scene*);
        typedef void(*SystemEventFunc)(Event&, Scene*);
    public:
        Scene();
        ~Scene();

        template<typename T>
        void add_system()
        {
            std::string system_name = get_type_name<T>();
            for (auto& [name, pos] : m_systems) {
                if (name == system_name) {
                    return;
                }
            }
            m_systems.push_back({system_name, { -1, -1 }});
            add_on_update<T>(system_name);
            add_on_event<T>(system_name);
        }

        template<typename T>
        void remove_system()
        {
            std::string system_name = get_type_name<T>();
            bool is_update_deleted = false;
            bool is_event_deleted = false;
            for (auto iter = m_systems.begin(); iter != m_systems.end(); iter++) {
                auto& [name, pos] = *iter;
                if (name == system_name) {
                    if (pos.first >= 0) {
                        m_system_update_funcs.erase(m_system_update_funcs.begin() + pos.first);
                        is_update_deleted = true;
                    }
                    if (pos.second >= 0) {
                        m_system_event_funcs.erase(m_system_event_funcs.begin() + pos.second);
                        is_event_deleted = true;
                    }
                    iter = m_systems.erase(iter);
                    if (iter == m_systems.end()) break;
                }
                if (is_update_deleted && iter->second.first > 0) {
                    iter->second.first --;
                }
                if (is_event_deleted && iter->second.second > 0) {
                    iter->second.second --;
                }
            }
        }

        template<typename... Args, typename F = std::function<void(Args&&...)>>
        void view_components(F func)
        {
            auto view = m_registry.view<Args...>();
            for (auto entity : view) {
                Entity e(entity, &m_registry);
                std::apply([&](auto&... args){func(e, args...);}, view.get(entity));
            }
        }

        Entity create_entity(uint32_t hint = 0);
        Entity get_entity(uint32_t handle)
        {
            if (m_registry.orphan((entt::entity)handle)) return Entity{};
            return Entity{(entt::entity)handle, &m_registry};
        }
        void delete_entity(Entity entity);

        void each_entity(std::function<void(Entity)> func);
        void each_system(std::function<void(std::string, int32_t, int32_t)> func);
        void change_system_order(uint32_t old_index, uint32_t new_index);

        void add_render_pass(const Ref<Material>& material, const std::vector<Ref<RenderTexture>>& attachments);
        void set_render_pass(uint32_t index, const Ref<Material>& material, const std::vector<Ref<RenderTexture>>& attachments);
        void set_frame_buffer(uint32_t index, const Ref<FrameBuffer>& frame_buffer);
        const std::vector<std::pair<Ref<Material>, Ref<FrameBuffer>>>& get_render_passes() { return m_render_passes; }

        void on_update(Timestep ts);
        void on_event(Event& e);
    private:
        entt::registry m_registry;
        std::vector<std::pair<std::string, std::pair<int32_t, int32_t>>> m_systems;
        std::vector<SystemUpdateFunc> m_system_update_funcs;
        std::vector<SystemEventFunc> m_system_event_funcs;

        std::vector<std::pair<Ref<Material>, Ref<FrameBuffer>>> m_render_passes{{nullptr, nullptr}};
        
        template<typename T>
        constexpr auto add_on_update(std::string system_name) -> decltype(T::on_update, void())
        {
            m_systems[m_systems.size() - 1].second.first = m_system_update_funcs.size();
            m_system_update_funcs.push_back(T::on_update);
        }
        template<typename T>
        constexpr void add_on_update(...)
        {}
        template<typename T>
        constexpr auto add_on_event(std::string system_name) -> decltype(T::on_event, void())
        {
            m_systems[m_systems.size() - 1].second.second = m_system_event_funcs.size();
            m_system_event_funcs.push_back(T::on_event);
        }
        template<typename T>
        constexpr void add_on_event(...)
        {}
    };

}