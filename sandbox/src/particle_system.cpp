#include "particle_system.h"
#include <random>
#include <glm/gtc/constants.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>

class Random
{
public:
    static void init()
    {
        s_random_engine.seed(std::random_device()());
    }

    static float rand()
    {
        return (float)s_distribution(s_random_engine) / (float)std::numeric_limits<uint32_t>::max();
    }

private:
    static std::mt19937 s_random_engine;
    static std::uniform_int_distribution<std::mt19937::result_type> s_distribution;
};
std::mt19937 Random::s_random_engine;
std::uniform_int_distribution<std::mt19937::result_type> Random::s_distribution(0, std::numeric_limits<uint32_t>::max());

ParticleSystem::ParticleSystem(uint32_t max_particles) : m_pool_id(max_particles - 1)
{
    m_particle_pool.resize(max_particles);
}

void ParticleSystem::on_update(Yogi::Timestep ts)
{
    for (auto& particle : m_particle_pool) {
        if (!particle.active)
            continue;

        if (particle.life_remaining <= 0.0f) {
            particle.active = false;
            continue;
        }

        particle.life_remaining -= ts;
        particle.position += particle.velocity * (float)ts;
        particle.rotation += 0.01f * ts;
    }
}

void ParticleSystem::on_render(Yogi::OrthographicCamera& camera)
{
    // Yogi::Renderer2D::begin_scene(camera);
    for (auto& particle : m_particle_pool) {
        if (!particle.active)
            continue;

        // Fade away particles
        float life = particle.life_remaining / particle.life_time;
        glm::vec4 color = glm::lerp(particle.color_end, particle.color_begin, life);
        //color.a = color.a * life;

        float size = glm::lerp(particle.size_end, particle.size_begin, life);
        
        // Yogi::Renderer2D::draw_quad({particle.position.x, particle.position.y, 0.0f}, particle.rotation, {size, size}, color);
    }
    // Yogi::Renderer2D::end_scene();
}

void ParticleSystem::emit(const ParticleProps& particle_props)
{
    Particle& particle = m_particle_pool[m_pool_id];
    particle.active = true;
    particle.position = particle_props.position;
    particle.rotation = Random::rand() * 2.0f * glm::pi<float>();

    // velocity
    particle.velocity = particle_props.velocity;
    particle.velocity.x += particle_props.velocity_variation.x * (Random::rand() - 0.5f);
    particle.velocity.y += particle_props.velocity_variation.y * (Random::rand() - 0.5f);

    // Color
    particle.color_begin = particle_props.color_begin;
    particle.color_end = particle_props.color_end;

    particle.life_time = particle_props.life_time;
    particle.life_remaining = particle_props.life_time;
    particle.size_begin = particle_props.size_begin + particle_props.size_variation * (Random::rand() - 0.5f);
    particle.size_end = particle_props.size_end;

    m_pool_id = --m_pool_id % m_particle_pool.size();
}