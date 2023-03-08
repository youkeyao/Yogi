#pragma once

#include <engine.h>

struct ParticleProps
{
    glm::vec2 position;
    glm::vec2 velocity, velocity_variation;
    glm::vec4 color_begin, color_end;
    float size_begin, size_end, size_variation;
    float life_time = 1.0f;
};

class ParticleSystem
{
public:
    ParticleSystem(uint32_t max_particles = 1000);

    void on_update(Yogi::Timestep ts);
    // void on_render(Yogi::OrthographicCamera& camera);

    void emit(const ParticleProps& particle_props);
private:
    struct Particle
    {
        glm::vec2 position;
        glm::vec2 velocity;
        glm::vec4 color_begin, color_end;
        float rotation = 0.0f;
        float size_begin, size_end;

        float life_time = 1.0f;
        float life_remaining = 0.0f;

        bool active = false;
    };
    std::vector<Particle> m_particle_pool;
    uint32_t m_pool_id;
};