#pragma once

#include "renderer/orthographic_camera.h"
#include "renderer/texture.h"

namespace hazel {

    class Renderer2D
    {
    public:
        static void init();
        static void shutdown();

        static void begin_scene(const OrthographicCamera& camera);
        static void end_scene();

        // Primitives
        static void draw_quad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
        {
            draw_quad({ position.x, position.y, 1.0f }, 0, size, color);
        }
        static void draw_quad(const glm::vec2& position, const float rotation, const glm::vec2& size, const glm::vec4& color)
        {
            draw_quad({ position.x, position.y, 1.0f }, rotation, size, color);
        }
        static void draw_quad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
        {
            draw_quad(position, 0, size, color);
        }
        static void draw_quad(const glm::vec3& position, const float rotation, const glm::vec2& size, const glm::vec4& color);
        static void draw_quad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float texture_scale = 1.0f, const glm::vec4& color = glm::vec4(1.0f))
        {
            draw_quad({ position.x, position.y, 1.0f }, 0, size, texture, texture_scale, color);
        }
        static void draw_quad(const glm::vec2& position, const float rotation, const glm::vec2& size, const Ref<Texture2D>& texture, float texture_scale = 1.0f, const glm::vec4& color = glm::vec4(1.0f))
        {
            draw_quad({ position.x, position.y, 1.0f }, rotation, size, texture, texture_scale, color);
        }
        static void draw_quad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float texture_scale = 1.0f, const glm::vec4& color = glm::vec4(1.0f))
        {
            draw_quad(position, 0, size, texture, texture_scale, color);
        }
        static void draw_quad(const glm::vec3& position, const float rotation, const glm::vec2& size, const Ref<Texture2D>& texture, float texture_scale = 1.0f, const glm::vec4& color = glm::vec4(1.0f));
    };

}