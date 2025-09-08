#include "Sandbox2D.h"

// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>

Sandbox2D::Sandbox2D() : Layer("Sandbox 2D") {}

std::vector<uint32_t> ReadFile(const std::string& filepath)
{
    std::vector<uint32_t> buffer;
    std::ifstream         in(filepath, std::ios::ate | std::ios::binary);

    if (!in.is_open())
    {
        YG_CORE_ERROR("Could not open file '{0}'!", filepath);
        return buffer;
    }

    in.seekg(0, std::ios::end);
    buffer.resize(in.tellg() / sizeof(uint32_t));
    in.seekg(0, std::ios::beg);
    in.read((char*)buffer.data(), buffer.size() * sizeof(uint32_t));
    in.close();

    return buffer;
}

void Sandbox2D::OnAttach()
{
    YG_PROFILE_FUNCTION();

    Yogi::AssetManager::PushAssetSource(Yogi::CreateScope<Yogi::FileSystemSource>("Assets/"));
    Yogi::View<Yogi::ISwapChain> swapChain = Yogi::Application::GetInstance().GetSwapChain();

    m_world = Yogi::CreateScope<Yogi::World>();

    auto renderPass = Yogi::IRenderPass::Create(
        Yogi::RenderPassDesc{ { Yogi::AttachmentDesc{ swapChain->GetColorFormat(), Yogi::AttachmentUsage::Present } },
                              Yogi::AttachmentDesc{ swapChain->GetDepthFormat(),
                                                    Yogi::AttachmentUsage::DepthStencil,
                                                    Yogi::LoadOp::Clear,
                                                    Yogi::StoreOp::DontCare },
                              swapChain->GetNumSamples() });
    m_renderPass = Yogi::AssetManager::AddAsset<Yogi::IRenderPass>(std::move(renderPass), "RenderPasses/Default.rpass");
    auto shaderResourceBinding = Yogi::IShaderResourceBinding::Create(
        { Yogi::ShaderResourceAttribute{ 0, 1, Yogi::ShaderResourceType::Buffer, Yogi::ShaderStage::Vertex } });
    m_shaderResourceBinding = Yogi::AssetManager::AddAsset<Yogi::IShaderResourceBinding>(
        std::move(shaderResourceBinding), "ShaderResourceBindings/Default.srb");

    std::vector<Yogi::ShaderDesc> shaders = { { Yogi::ShaderStage::Vertex, ReadFile("BuildShader/Test.vert") },
                                              { Yogi::ShaderStage::Fragment, ReadFile("BuildShader/Test.frag") } };
    m_pipeline = Yogi::IPipeline::Create(
        Yogi::PipelineDesc{ shaders,
                            { Yogi::VertexAttribute{ "Pos", 0, 12, Yogi::ShaderElementType::Float3 },
                              Yogi::VertexAttribute{ "Color", 12, 16, Yogi::ShaderElementType::Float4 } },
                            m_shaderResourceBinding.Get(),
                            m_renderPass.Get(),
                            0,
                            Yogi::PrimitiveTopology::TriangleList });

    auto mesh     = Yogi::AssetManager::GetAsset<Yogi::Mesh>("Meshes/Cube.obj::cube");
    auto material = Yogi::CreateScope<Yogi::Material>(CreateView(m_pipeline));
    auto materialAsset = Yogi::AssetManager::AddAsset<Yogi::Material>(std::move(material), "Materials/Default.mat");

    m_world->CreateEntity().AddComponent<Yogi::MeshRendererComponent>(mesh, materialAsset);
    m_world->AddSystem<Yogi::ForwardRenderSystem>();

    // m_depth_texture = context->create_texture(
    //     Yogi::TextureDesc{ swap_chain->get_width(), swap_chain->get_height(), 1, 1,
    //     Yogi::ITexture::Format::D32_FLOAT,
    //                        Yogi::ITexture::Usage::DepthStencil });
    // m_pipeline = context->create_pipeline(
    //     Yogi::PipelineDesc{
    //         shaders,
    //         { Yogi::VertexAttribute{ "in_position", 0, 12, Yogi::ShaderElementType::Float3 },
    //           Yogi::VertexAttribute{ "in_color", 12, 15, Yogi::ShaderElementType::Float4 } },
    //         { Yogi::ITexture::Format::R8G8B8A8_SRGB },
    //         Yogi::ITexture::Format::D32_FLOAT,
    //         Yogi::PrimitiveTopology::TriangleList
    // });

    // m_scene                        = Yogi::CreateScope<Yogi::Scene>();
    // Yogi::Ref<Yogi::Mesh>     quad = Yogi::MeshManager::get_mesh("quad");
    // Yogi::Ref<Yogi::Material> mat1 = Yogi::MaterialManager::get_material("default");

    // m_scene->add_system<Yogi::RenderSystem>();
    // m_scene->add_system<Yogi::PhysicsSystem>();

    // checker                                                     = m_scene->create_entity();
    // checker.add_component<Yogi::TransformComponent>().transform = glm::translate(glm::mat4(1.0f), {0, -0.3, 0});
    // checker.add_component<Yogi::MeshRendererComponent>(quad, mat1);

    // Yogi::Entity e                                        = m_scene->create_entity();
    // e.add_component<Yogi::TransformComponent>().transform = glm::translate(glm::mat4(1.0f), {0, 0, 0.05});
    // e.add_component<Yogi::MeshRendererComponent>(quad, mat1);

    // e = m_scene->create_entity();
    // e.add_component<Yogi::TransformComponent>().transform =
    //     glm::inverse(glm::lookAt(glm::vec3{2, 1, 2}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0}));
    // auto& camera        = e.add_component<Yogi::CameraComponent>();
    // camera.aspect_ratio = 1280.0f / 720.0f;
    // camera.is_ortho     = false;

    // e                                                     = m_scene->create_entity();
    // e.add_component<Yogi::TransformComponent>().transform = glm::translate(glm::mat4(1.0f), {0.4, 0, 0.1});
    // e.add_component<Yogi::MeshRendererComponent>(quad, mat1);

    // for (int32_t i = 0; i < 100; i++)
    // {
    //     for (int32_t j = 0; j < 100; j++)
    //     {
    //         e = m_scene->create_entity();
    //         e.add_component<Yogi::TransformComponent>().transform =
    //             glm::translate(glm::mat4(1.0f), glm::vec3(0.02 * i - 1, 0.02 * j - 1, 0.11)) *
    //             glm::scale(glm::mat4(1.0f), glm::vec3(0.01, 0.01, 0.1));
    //         e.add_component<Yogi::MeshRendererComponent>(quad, mat1);
    //     }
    // }

    // e                                                     = m_scene->create_entity();
    // e.add_component<Yogi::TransformComponent>().transform = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 1));
    // e.add_component<Yogi::PointLightComponent>();

    // e                                                     = m_scene->create_entity();
    // e.add_component<Yogi::TransformComponent>().transform = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 5));
    // e.add_component<Yogi::DirectionalLightComponent>();

    // e = m_scene->create_entity();
    // e.add_component<Yogi::TransformComponent>();
    // e.add_component<Yogi::SkyboxComponent>().material = Yogi::MaterialManager::get_material("skybox");

    // e = m_scene->create_entity();
    // e.add_component<Yogi::TransformComponent>().transform =
    //     glm::translate(glm::mat4(1.0f), glm::vec3(0, 0.5, 1)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    // e.add_component<Yogi::MeshRendererComponent>(Yogi::MeshManager::get_mesh("cube"), mat1);
    // e.add_component<Yogi::RigidBodyComponent>();

    // e = m_scene->create_entity();
    // e.add_component<Yogi::TransformComponent>().transform =
    //     glm::rotate(glm::mat4(1.0f), glm::radians(-10.0f), glm::vec3(1, 0, 0)) *
    //     glm::translate(glm::mat4(1.0f), glm::vec3(0, -1.0f, 0)) *
    //     glm::scale(glm::mat4(1.0f), glm::vec3(4.0f, 1.0f, 4.0f));
    // e.add_component<Yogi::MeshRendererComponent>(Yogi::MeshManager::get_mesh("cube"), mat1);
    // e.add_component<Yogi::RigidBodyComponent>().is_static = true;
}

void Sandbox2D::OnDetach() { YG_PROFILE_FUNCTION(); }

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
        // Yogi::View<Yogi::ISwapChain>    swapChain     = Yogi::Application::GetInstance().GetSwapChain();
        // Yogi::View<Yogi::ITexture>      currentTarget = swapChain->GetCurrentTarget();
        // Yogi::View<Yogi::ITexture>      currentDepth  = swapChain->GetCurrentDepth();
        // Yogi::Scope<Yogi::IFrameBuffer> frameBuffer   = Yogi::IFrameBuffer::Create(Yogi::FrameBufferDesc{
        //     swapChain->GetWidth(),
        //     swapChain->GetHeight(),
        //     CreateView(m_renderPass),
        //       { currentTarget },
        //     currentDepth,
        // });

        // m_transform = Yogi::Matrix4::Rotation(0, (float)ts * 20.0f, 0) * m_transform;
        // Yogi::Matrix4 transform =
        //     Yogi::MathUtils::Perspective(
        //         45.0f, (float)swapChain->GetWidth() / (float)swapChain->GetHeight(), 0.1f, 100.0f) *
        //     Yogi::Matrix4::Translation(Yogi::Vector3{ 0, 0, -5 }) * m_transform;
        // m_uniformBuffer->UpdateData(&transform, sizeof(Yogi::Matrix4), 0);

        // Yogi::Scope<Yogi::ICommandBuffer> commandBuffer = Yogi::ICommandBuffer::Create(
        //     Yogi::CommandBufferDesc{ Yogi::CommandBufferUsage::OneTimeSubmit, Yogi::SubmitQueue::Graphics });
        // commandBuffer->Begin();
        // commandBuffer->BeginRenderPass(
        //     CreateView(frameBuffer), { Yogi::ClearValue{ 0.1f, 0.1f, 0.1f, 1.0f } }, Yogi::ClearValue{ 1.0f, 0 });
        // commandBuffer->SetVertexBuffer(CreateView(m_vertexBuffer));
        // commandBuffer->SetIndexBuffer(CreateView(m_indexBuffer));
        // commandBuffer->SetPipeline(CreateView(m_pipeline));
        // commandBuffer->SetShaderResourceBinding(CreateView(m_shaderResourceBinding));
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

void Sandbox2D::OnEvent(Yogi::Event& e)
{
    // m_scene->on_event(e);
}