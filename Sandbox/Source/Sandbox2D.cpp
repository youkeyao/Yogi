#include "Sandbox2D.h"

std::vector<uint8_t> ReadFile(const std::string& filepath)
{
    std::vector<uint8_t> buffer;
    std::ifstream        in(filepath, std::ios::ate | std::ios::binary);

    if (!in.is_open())
    {
        YG_CORE_ERROR("Could not open file '{0}'!", filepath);
        return buffer;
    }

    in.seekg(0, std::ios::end);
    buffer.resize(in.tellg() / sizeof(uint8_t));
    in.seekg(0, std::ios::beg);
    in.read((char*)buffer.data(), buffer.size() * sizeof(uint8_t));
    in.close();

    return buffer;
}

Sandbox2D::Sandbox2D() : Layer("Sandbox 2D")
{
    YG_PROFILE_FUNCTION();

    Yogi::AssetManager::PushAssetSource<Yogi::FileSystemSource>(".");

    auto& swapChain = Yogi::Application::GetInstance().GetSwapChain();

    auto renderPass = Yogi::ResourceManager::GetResource<Yogi::IRenderPass>(
        Yogi::RenderPassDesc{ { Yogi::AttachmentDesc{ swapChain->GetColorFormat(), Yogi::AttachmentUsage::Present } },
                              Yogi::AttachmentDesc{ swapChain->GetDepthFormat(),
                                                    Yogi::AttachmentUsage::DepthStencil,
                                                    Yogi::LoadOp::Clear,
                                                    Yogi::StoreOp::DontCare },
                              swapChain->GetNumSamples() });

    auto shaderResourceBinding =
        Yogi::ResourceManager::GetResource<Yogi::IShaderResourceBinding>(std::vector<Yogi::ShaderResourceAttribute>{
            Yogi::ShaderResourceAttribute{ 0, 1, Yogi::ShaderResourceType::Buffer, Yogi::ShaderStage::Vertex } });

    Yogi::Handle<Yogi::ShaderDesc> vertexShader =
        Yogi::Handle<Yogi::ShaderDesc>::Create(Yogi::ShaderStage::Vertex, ReadFile("Assets/Shaders/Test.vert"));
    Yogi::Handle<Yogi::ShaderDesc> fragmentShader =
        Yogi::Handle<Yogi::ShaderDesc>::Create(Yogi::ShaderStage::Fragment, ReadFile("Assets/Shaders/Test.frag"));
    std::vector<Yogi::Ref<Yogi::ShaderDesc>> shaders = { Yogi::Ref<Yogi::ShaderDesc>::Create(vertexShader),
                                                         Yogi::Ref<Yogi::ShaderDesc>::Create(fragmentShader) };

    auto pipeline = Yogi::ResourceManager::GetResource<Yogi::IPipeline>(
        Yogi::PipelineDesc{ shaders,
                            { Yogi::VertexAttribute{ "a_Position", 0, 12, Yogi::ShaderElementType::Float3 },
                              Yogi::VertexAttribute{ "a_TexCoord", 12, 8, Yogi::ShaderElementType::Float2 } },
                            shaderResourceBinding,
                            renderPass,
                            0,
                            Yogi::PrimitiveTopology::TriangleList });

    m_world = Yogi::Handle<Yogi::World>::Create();
    m_world->AddSystem<Yogi::ForwardRenderSystem>();

    auto  entity                 = m_world->CreateEntity();
    auto& transform              = entity.AddComponent<Yogi::TransformComponent>();
    transform.Transform.Position = { 0, 0, 5 };
    auto& camera                 = entity.AddComponent<Yogi::CameraComponent>();

    m_box = m_world->CreateEntity();
    m_box.AddComponent<Yogi::TransformComponent>();
    auto& meshRenderer    = m_box.AddComponent<Yogi::MeshRendererComponent>();
    meshRenderer.Mesh     = Yogi::AssetManager::GetAsset<Yogi::Mesh>("Assets/Meshes/Cube.obj::cube");
    meshRenderer.Material = Yogi::ResourceManager::GetResource<Yogi::Material>();
    meshRenderer.Material->SetPipeline(pipeline);
}

Sandbox2D::~Sandbox2D() { m_world = nullptr; }

void Sandbox2D::OnUpdate(Yogi::Timestep ts)
{
    YG_PROFILE_FUNCTION();
    YG_CORE_INFO("{0}", 1.0f / ts);

    // m_camera_controller.on_update(ts);

    // Yogi::Renderer2D::reset_stats();
    {
        // YG_PROFILE_SCOPE("Render prep");
        // Yogi::RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        // Yogi::RenderCommand::clear();
    }

    {
        YG_PROFILE_SCOPE("Render draw");
        auto& transform    = m_box.GetComponent<Yogi::TransformComponent>().Transform;
        transform.Rotation = Yogi::Quaternion::AngleAxis((float)ts * 20.0f, Yogi::Vector3::Up()) * transform.Rotation;
        m_world->OnUpdate(ts);
        // auto&                            swapChain     = Yogi::Application::GetInstance().GetSwapChain();
        // auto                             currentTarget = swapChain->GetCurrentTarget();
        // auto                             currentDepth  = swapChain->GetCurrentDepth();
        // Yogi::Handle<Yogi::IFrameBuffer> frameBuffer   = Yogi::IFrameBuffer::Create(Yogi::FrameBufferDesc{
        //     swapChain->GetWidth(),
        //     swapChain->GetHeight(),
        //     Yogi::Ref<Yogi::IRenderPass>::Create(m_renderPass),
        //       { currentTarget },
        //     currentDepth,
        // });

        // m_transform = Yogi::Matrix4::Rotation(0, (float)ts * 20.0f, 0) * m_transform;
        // Yogi::Matrix4 transform =
        //     Yogi::MathUtils::Perspective(
        //         45.0f, (float)swapChain->GetWidth() / (float)swapChain->GetHeight(), 0.1f, 100.0f) *
        //     Yogi::Matrix4::Translation(Yogi::Vector3{ 0, 0, -5 }) * m_transform;
        // m_uniformBuffer->UpdateData(&transform, sizeof(Yogi::Matrix4), 0);

        // Yogi::Handle<Yogi::ICommandBuffer> commandBuffer = Yogi::ICommandBuffer::Create(
        //     Yogi::CommandBufferDesc{ Yogi::CommandBufferUsage::OneTimeSubmit, Yogi::SubmitQueue::Graphics });
        // commandBuffer->Begin();
        // commandBuffer->BeginRenderPass(Yogi::Ref<Yogi::IFrameBuffer>::Create(frameBuffer),
        //                                { Yogi::ClearValue{ 0.1f, 0.1f, 0.1f, 1.0f } },
        //                                Yogi::ClearValue{ 1.0f, 0 });
        // commandBuffer->SetVertexBuffer(Yogi::Ref<Yogi::IBuffer>::Create(m_vertexBuffer));
        // commandBuffer->SetIndexBuffer(Yogi::Ref<Yogi::IBuffer>::Create(m_indexBuffer));
        // commandBuffer->SetPipeline(Yogi::Ref<Yogi::IPipeline>::Create(m_pipeline));
        // commandBuffer->SetShaderResourceBinding(
        //     Yogi::Ref<Yogi::IShaderResourceBinding>::Create(m_shaderResourceBinding));
        // commandBuffer->SetViewport({ 0, 0, (float)swapChain->GetWidth(), (float)swapChain->GetHeight() });
        // commandBuffer->SetScissor({ 0, 0, swapChain->GetWidth(), swapChain->GetHeight() });
        // commandBuffer->DrawIndexed(m_indexBuffer->GetSize() / sizeof(uint32_t), 1, 0, 0, 0);
        // commandBuffer->EndRenderPass();
        // commandBuffer->End();
        // commandBuffer->Submit();

        // Yogi::Renderer2D::begin_scene(m_camera_controller.get_camera());
        // Yogi::Renderer2D::draw_quad({-1.0f, 0.0f}, glm::radians(45.0f), {0.8f, 0.8f}, {0.8f, 0.2f, 0.3f, 1.0f});
        // Yogi::Renderer2D::draw_quad({0.5f, -0.5f}, {0.5f, 0.75f}, {0.2f, 0.3f, 0.8f, 1.0f});
        // Yogi::Renderer2D::draw_quad({-0.5f, 0.5f}, {0.5f, 0.75f}, {0.2f, 0.8f, 0.3f, 1.0f});
        // Yogi::Renderer2D::draw_quad({ 0.0f, 0.0f, -0.1f }, { 20.0f, 20.0f }, m_checkerboard_texture, {{0.0f, 0.0f},
        // {10.0f, 10.0f}}, m_square_color); Yogi::Renderer2D::draw_quad({ 0.0f, 0.0f, 0.1f }, glm::radians(45.0f),
        // { 1.0f, 1.0f
        // }, m_checkerboard_texture, {{0.0f, 0.0f}, {5.0f, 5.0f}});

        // for (float y = -5.0f; y < 5.0f; y += 0.5f) {
        //     for (float x = -5.0f; x < 5.0f; x += 0.5f) {
        //         Yogi::Renderer2D::draw_quad({x, y}, {0.45f, 0.45f}, {(x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f,
        //         0.7f});
        //     }
        // }
        // auto& transform = checker.get_component<Yogi::TransformComponent>().transform;
        // transform       = glm::rotate(glm::mat4(1.0f), (float)ts, glm::vec3{0, 1, 0}) * (glm::mat4)transform;
        // m_scene->on_update(ts);
        // Yogi::Renderer2D::end_scene();
    }

    // if (Yogi::Input::is_mouse_button_pressed(YG_MOUSE_BUTTON_LEFT)) {
    //     auto [x, y] = Yogi::Input::get_mouse_position();
    //     auto width = Yogi::Application::get().get_window().get_width();
    //     auto height = Yogi::Application::get().get_window().get_height();

    //     glm::vec2 bounds = { 2 * m_camera_controller.zoom_level() * 1280.0f / 720.0f, 2 *
    //     m_camera_controller.zoom_level() }; auto pos = m_camera_controller.get_camera().get_position(); x = (x /
    //     width) * bounds.x - bounds.x * 0.5f; y = bounds.y * 0.5f - (y / height) * bounds.y; m_particle_props.position
    //     = { x + pos.x, y + pos.y }; for (int i = 0; i < 5; i++)
    //         m_particle_system.emit(m_particle_props);
    // }

    // m_particle_system.on_update(ts);
    // m_particle_system.on_render(m_camera_controller.get_camera());
}

void Sandbox2D::OnEvent(Yogi::Event& e) { m_world->OnEvent(e); }