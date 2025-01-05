#pragma once

#include "runtime/events/application_event.h"
#include "runtime/renderer/frame_buffer.h"
#include "runtime/scene/components.h"
#include "runtime/scene/scene.h"
#include "runtime/scene/system_base.h"

namespace Yogi {

class RenderSystem : public SystemBase
{
private:
    struct ShadowData
    {
        glm::mat4 light_space_matrix = glm::mat4(1.0f);
        Transform light_transform = glm::mat4(1.0f);
        uint32_t  shadow_pool_index = 0;
    };

public:
    RenderSystem();
    ~RenderSystem();

    void on_update(Timestep ts, Scene *scene) override;
    void set_light(Scene *scene);
    void render_camera(const CameraComponent &camera, const TransformComponent &transform, Scene *scene);

    void on_event(Event &e, Scene *scene) override;
    bool on_window_resized(WindowResizeEvent &e, Scene *scene);

    static void set_default_frame_buffer(const Ref<FrameBuffer> &frame_buffer);

private:
    uint32_t allocate_shadow_frame_buffer();

private:
    int32_t                                                                m_shadow_map_size = 2048;
    std::vector<std::pair<DirectionalLightComponent, ShadowData>>          m_directional_lights;
    std::vector<std::pair<SpotLightComponent, ShadowData>>                 m_spot_lights;
    std::vector<std::pair<PointLightComponent, std::array<ShadowData, 6>>> m_point_lights;
    std::vector<Ref<FrameBuffer>>                                          m_shadow_frame_buffer_pool;
    uint32_t                                                               m_shadow_frame_buffer_pool_index = 0;
    static int                                                             s_width;
    static int                                                             s_height;
    static FrameBuffer                                                    *s_frame_buffer;
};

}  // namespace Yogi