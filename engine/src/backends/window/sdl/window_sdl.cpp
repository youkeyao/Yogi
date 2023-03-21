#include "backends/window/sdl/window_sdl.h"
#include "backends/window/sdl/sdl_to_yg_codes.h"

namespace Yogi {

    static void sdl_error_callback(int error, const char* description) {
        YG_CORE_ERROR("SDL Error ({0}): {1}", error, description);
    }

    Scope<Window> Window::create(const WindowProps& props)
    {
        return CreateScope<WindowSDL>(props);
    }

    WindowSDL::WindowSDL(const WindowProps& props)
    {
        m_data.title = props.title;
        m_data.width = props.width;
        m_data.height = props.height;
    }

    void WindowSDL::init()
    {
        YG_CORE_INFO("Creating Window {0} ({1} {2})", m_data.title, m_data.width, m_data.height);

        int success = SDL_Init(SDL_INIT_VIDEO);
        YG_CORE_ASSERT(success >= 0, "Could not initialize SDL!");
        #if YG_RENDERER_API == YG_RENDERER_OPENGL
            m_window = SDL_CreateWindow(m_data.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_data.width, m_data.height, SDL_WINDOW_OPENGL );
        #elif YG_RENDERER_API == YG_RENDERER_VULKAN
            m_window = SDL_CreateWindow(m_data.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_data.width, m_data.height, SDL_WINDOW_VULKAN );
        #endif

        m_context = GraphicsContext::create(this);
        m_context->init();
    }

    WindowSDL::~WindowSDL()
    {
        #if YG_RENDERER_API == YG_RENDERER_OPENGL
            SDL_GL_DeleteContext(SDL_GL_GetCurrentContext());
        #endif
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }

    void WindowSDL::on_update()
    {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    m_data.width = e.window.data1;
                    m_data.height = e.window.data2;

                    WindowResizeEvent event(e.window.data1, e.window.data2, &e);
                    m_data.event_callback(event);
                }
                else if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                    WindowCloseEvent event(&e);
                    m_data.event_callback(event);
                }
            }
            else if (e.type == SDL_KEYDOWN) {
                if (!e.key.repeat) {
                    KeyPressedEvent event(sdl_to_yg_codes(e.key.keysym.scancode), 0, &e);
                    m_data.event_callback(event);
                }
                else {
                    KeyPressedEvent event(sdl_to_yg_codes(e.key.keysym.scancode), 1, &e);
                    m_data.event_callback(event);
                }
            }
            else if (e.type == SDL_KEYUP) {
                KeyReleasedEvent event(sdl_to_yg_codes(e.key.keysym.scancode), &e);
                m_data.event_callback(event);
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                MouseButtonPressedEvent event(sdl_to_yg_codes(e.button.button), &e);
                m_data.event_callback(event);
            }
            else if (e.type == SDL_MOUSEBUTTONUP) {
                MouseButtonReleasedEvent event(sdl_to_yg_codes(e.button.button), &e);
                m_data.event_callback(event);
            }
            else if (e.type == SDL_MOUSEWHEEL) {
                MouseScrolledEvent event(e.wheel.x, e.wheel.y, &e);
                m_data.event_callback(event);
            }
            else if (e.type == SDL_MOUSEMOTION) {
                MouseMovedEvent event(e.motion.x, e.motion.y, &e);
                m_data.event_callback(event);
            }
        }
        m_context->swap_buffers();
    }

}