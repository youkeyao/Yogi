#pragma once

#include "base/renderer/orthographic_camera.h"
#include "base/renderer/texture.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Yogi {

    class Renderer2D
    {
    public:
        static void init();
        static void shutdown();

        static void set_projection_view_matrix(glm::mat4 projection_view_matrix);
        static void flush();

        // Primitives
        static void draw_quad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, 0.0f })
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, nullptr, {{0.0f, 0.0f}, {1.0f, 1.0f}}, color);
        }
        static void draw_quad(const glm::vec2& position, const float rotation, const glm::vec2& size, const glm::vec4& color)
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, 0.0f })
                * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, nullptr, {{0.0f, 0.0f}, {1.0f, 1.0f}}, color);
        }
        static void draw_quad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, nullptr, {{0.0f, 0.0f}, {1.0f, 1.0f}}, color);
        }
        static void draw_quad(const glm::vec3& position, const float rotation, const glm::vec2& size, const glm::vec4& color)
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
                * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, nullptr, {{0.0f, 0.0f}, {1.0f, 1.0f}}, color);
        }
        static void draw_quad(const glm::mat4& transform, const glm::vec4& color)
        {
            draw_quad(transform, nullptr, {{0.0f, 0.0f}, {1.0f, 1.0f}}, color);
        }
        static void draw_quad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec4& tint_color = glm::vec4(1.0f))
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, 0.0f })
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, texture, {{0.0f, 0.0f}, {1.0f, 1.0f}}, tint_color);
        }
        static void draw_quad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, const std::pair<glm::vec2, glm::vec2>& texcoords, const glm::vec4& tint_color = glm::vec4(1.0f))
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, 0.0f })
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, texture, texcoords, tint_color);
        }
        static void draw_quad(const glm::vec2& position, const float rotation, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec4& tint_color = glm::vec4(1.0f))
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, 0.0f })
                * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, texture, {{0.0f, 0.0f}, {1.0f, 1.0f}}, tint_color);
        }
        static void draw_quad(const glm::vec2& position, const float rotation, const glm::vec2& size, const Ref<Texture2D>& texture, const std::pair<glm::vec2, glm::vec2>& texcoords, const glm::vec4& tint_color = glm::vec4(1.0f))
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, 0.0f })
                * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, texture, texcoords, tint_color);
        }
        static void draw_quad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec4& tint_color = glm::vec4(1.0f))
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, texture, {{0.0f, 0.0f}, {1.0f, 1.0f}}, tint_color);
        }
        static void draw_quad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, const std::pair<glm::vec2, glm::vec2>& texcoords, const glm::vec4& tint_color = glm::vec4(1.0f))
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, texture, texcoords, tint_color);
        }
        static void draw_quad(const glm::vec3& position, const float rotation, const glm::vec2& size, const Ref<Texture2D>& texture, const glm::vec4& tint_color = glm::vec4(1.0f))
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
                * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, texture, {{0.0f, 0.0f}, {1.0f, 1.0f}}, tint_color);
        }
        static void draw_quad(const glm::vec3& position, const float rotation, const glm::vec2& size, const Ref<Texture2D>& texture, const std::pair<glm::vec2, glm::vec2>& texcoords, const glm::vec4& tint_color = glm::vec4(1.0f))
        {
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
                * glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
                * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
            draw_quad(transform, texture, texcoords, tint_color);
        }
        static void draw_quad(const glm::mat4& transform, const Ref<Texture2D>& texture, const glm::vec4& tint_color = glm::vec4(1.0f))
        {
            draw_quad(transform, texture, {{0.0f, 0.0f}, {1.0f, 1.0f}}, tint_color);
        }
        static void draw_quad(const glm::mat4& transform, const Ref<Texture2D>& texture, const std::pair<glm::vec2, glm::vec2>& texcoords, const glm::vec4& tint_color = glm::vec4(1.0f));

        struct Statistics
        {
            uint32_t draw_calls = 0;
            uint32_t quad_count = 0;
            uint32_t get_total_vertex_count() const { return quad_count * 4; }
			uint32_t get_total_index_count() const { return quad_count * 6; }
        };
        static void reset_stats();
		static Statistics get_stats();
    };

}