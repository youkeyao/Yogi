#include "Sandbox2D.h"
#include "Renderer/ShaderData.h"

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

    m_world = Yogi::Owner<Yogi::World>::Create();
    m_world->AddSystem<Yogi::ForwardRenderSystem>();

    std::vector<Yogi::WRef<Yogi::Mesh>> meshes = {
        Yogi::AssetManager::AcquireAsset<Yogi::Mesh>("EngineAssets/Meshes/Armadillo.obj::defaultobject"),
        Yogi::AssetManager::AcquireAsset<Yogi::Mesh>("EngineAssets/Meshes/Cube.obj::cube"),
        Yogi::AssetManager::AcquireAsset<Yogi::Mesh>("EngineAssets/Meshes/Bunny.obj::defaultobject")
    };

    // [DEBUG isolation #3] frag-side BDA + sampler chain confirmed clean
    // (single-material + full Test.frag rendered stably). Re-enable multi-
    // material so we can see if the flicker comes back -- if yes, the bug
    // is exclusive to the multi-material path: 4 MaterialData uploads,
    // RegisterTexture-driven slot allocation in u_textures[], non-zero
    // v_MaterialIndex values, or some interaction between them.
    Yogi::WRef<Yogi::ITexture> texCheckerboard =
        Yogi::AssetManager::AcquireAsset<Yogi::ITexture>("EngineAssets/Textures/Checkerboard.png");
    Yogi::WRef<Yogi::ITexture> texChernoLogo =
        Yogi::AssetManager::AcquireAsset<Yogi::ITexture>("EngineAssets/Textures/ChernoLogo.png");
    Yogi::WRef<Yogi::ITexture> texSkybox =
        Yogi::AssetManager::AcquireAsset<Yogi::ITexture>("EngineAssets/Textures/Skybox.png");

    auto makeStandardMaterial = [](const Yogi::WRef<Yogi::ITexture>& tex, const Yogi::Vector4& tint) {
        auto m = Yogi::ResourceManager::CreateResource<Yogi::Material>();
        m->Schema =
            Yogi::AssetManager::AcquireAsset<Yogi::MaterialSchema>("EngineAssets/Shaders/Materials/Standard.slang");
        m->Params["AlbedoTexture"] = tex;
        m->Params["BaseColor"]     = tint;
        return m;
    };
    auto makeUnlitMaterial = [](const Yogi::WRef<Yogi::ITexture>& tex, const Yogi::Vector4& tint) {
        auto m = Yogi::ResourceManager::CreateResource<Yogi::Material>();
        m->Schema =
            Yogi::AssetManager::AcquireAsset<Yogi::MaterialSchema>("EngineAssets/Shaders/Materials/Unlit.slang");
        m->Params["AlbedoTexture"] = tex;
        // UnlitMaterial's authoring spelling is `Tint` (BaseColor in the
        // shared MaterialData slab row, but the impl reads it as Tint --
        // this is an alias-by-offset deal). We feed via the runtime param
        // name the IMaterial impl uses; reflection lets the editor see
        // "Tint" instead of "BaseColor".
        m->Params["Tint"] = tint;
        return m;
    };

    std::vector<Yogi::WRef<Yogi::Material>> materials = {
        // Standard: lit by NormalShade -- responds to surface orientation.
        makeStandardMaterial(texCheckerboard, Yogi::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }),
        makeStandardMaterial(texChernoLogo, Yogi::Vector4{ 1.0f, 0.6f, 0.6f, 1.0f }),
        // Unlit: full-bright tint*albedo, no normal contribution. Visually
        // pops next to Standard meshes in the same scene -- the side-by-side
        // is the easiest way to tell the bucket / specialization is real.
        makeUnlitMaterial(texCheckerboard, Yogi::Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }),
        makeUnlitMaterial(texSkybox, Yogi::Vector4{ 0.4f, 1.0f, 0.6f, 1.0f }),
    };

    srand(41);
    for (int i = 0; i < 1000; i++)
    {
        float x = float(rand()) / RAND_MAX * 4.0f - 2.0f;
        float y = float(rand()) / RAND_MAX * 4.0f - 2.0f;
        float z = float(rand()) / RAND_MAX * 4.0f - 2.0f;

        float x_axis = float(rand()) / RAND_MAX * 2 - 1.0f;
        float y_axis = float(rand()) / RAND_MAX * 2 - 1.0f;
        float z_axis = float(rand()) / RAND_MAX * 2 - 1.0f;
        float angle  = float(rand()) / RAND_MAX * 360.0f;

        float x_scale = float(rand()) / RAND_MAX / 2;
        float y_scale = float(rand()) / RAND_MAX / 2;
        float z_scale = float(rand()) / RAND_MAX / 2;

        m_box                    = m_world->CreateEntity();
        auto& entityTransform    = m_box.AddComponent<Yogi::TransformComponent>().Transform;
        entityTransform.Position = { x, y, z };
        entityTransform.Rotation =
            Yogi::Quaternion::AngleAxis(angle, Yogi::Vector3(x_axis, y_axis, z_axis).Normalized());
        entityTransform.Scale = { x_scale, y_scale, z_scale };
        auto& meshRenderer    = m_box.AddComponent<Yogi::MeshRendererComponent>();
        meshRenderer.Mesh     = meshes[rand() % meshes.size()];
        // 4 unique Material*'s -> getOrAddMaterial dedupes them into 4
        // MaterialData uploads per frame; 3 unique ITextures register into
        // slots 1..3 of u_textures[] via FRS::RegisterTexture; the 10000
        // instances cost zero extra material work.
        meshRenderer.Material = materials[rand() % materials.size()];
    }

    m_box                        = m_world->CreateEntity();
    auto& transform              = m_box.AddComponent<Yogi::TransformComponent>();
    transform.Transform.Position = { 0, 0, 10 };
    auto& camera                 = m_box.AddComponent<Yogi::CameraComponent>();

    // m_box = m_world->CreateEntity();
    // m_box.AddComponent<Yogi::TransformComponent>();
    // auto& meshRenderer = m_box.AddComponent<Yogi::MeshRendererComponent>();
    // meshRenderer.Mesh  = Yogi::AssetManager::AcquireAsset<Yogi::Mesh>("EngineAssets/Meshes/Armadillo.obj::defaultobject");
    // Yogi::WRef<Yogi::Material> material = Yogi::ResourceManager::GetResource<Yogi::Material>();
    // material->AddPass(Yogi::Material::MaterialPass{ pipeline, {} });
    // meshRenderer.Material = material;

    // entity                                                             = m_world->CreateEntity();
    // entity.AddComponent<Yogi::TransformComponent>().Transform.Position = { 2, 0, 0 };
    // auto& mr    = entity.AddComponent<Yogi::MeshRendererComponent>();
    // mr.Mesh     = Yogi::AssetManager::AcquireAsset<Yogi::Mesh>("EngineAssets/Meshes/Cube.obj::cube");
    // mr.Material = material;

    // entity                        = m_world->CreateEntity();
    // auto& transform2              = entity.AddComponent<Yogi::TransformComponent>();
    // transform2.Transform.Position = { -2, 0, 0 };
    // transform2.Transform.Scale    = { 5.f, 5.f, 5.f };
    // auto& mr1                     = entity.AddComponent<Yogi::MeshRendererComponent>();
    // mr1.Mesh     = Yogi::AssetManager::AcquireAsset<Yogi::Mesh>("EngineAssets/Meshes/Bunny.obj::defaultobject");
    // mr1.Material = material;
}

Sandbox2D::~Sandbox2D()
{
    m_world = nullptr;
}

void Sandbox2D::OnUpdate(Yogi::Timestep ts)
{
    YG_PROFILE_FUNCTION();
    // YG_CORE_INFO("{0}", 1.0f / ts);

    // m_camera_controller.on_update(ts);

    // Yogi::Renderer2D::reset_stats();
    {
        // YG_PROFILE_SCOPE("Render prep");
        // Yogi::RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        // Yogi::RenderCommand::clear();
    }

    {
        YG_PROFILE_SCOPE("Render draw");
        auto& transform = m_box.GetComponent<Yogi::TransformComponent>().Transform;
        if (Yogi::Input::IsKeyPressed(YG_KEY_A))
        {
            transform.Position -= 2.0f * (float)ts * (transform.Rotation * Yogi::Vector3::Right());
        }
        if (Yogi::Input::IsKeyPressed(YG_KEY_D))
        {
            transform.Position += 2.0f * (float)ts * (transform.Rotation * Yogi::Vector3::Right());
        }
        if (Yogi::Input::IsKeyPressed(YG_KEY_W))
        {
            transform.Position -= 2.0f * (float)ts * (transform.Rotation * Yogi::Vector3::Backward());
        }
        if (Yogi::Input::IsKeyPressed(YG_KEY_S))
        {
            transform.Position += 2.0f * (float)ts * (transform.Rotation * Yogi::Vector3::Backward());
        }
        if (firstMouse)
        {
            lastX      = Yogi::Input::GetMouseX();
            lastY      = Yogi::Input::GetMouseY();
            firstMouse = false;
        }
        // Mouse delta is already a per-frame quantity (pixels travelled this frame),
        // so we must NOT multiply by ts -- doing so double-integrates over time and
        // produces a huge view jump on any frame stall (large ts * large delta).
        // sensitivity is just a constant pixels-to-radians scale.
        const float kMouseSensitivity = 0.02f;
        transform.Rotation = Yogi::Quaternion::AngleAxis(kMouseSensitivity * (lastX - Yogi::Input::GetMouseX()),
                                                         transform.Rotation * Yogi::Vector3::Up()) *
            transform.Rotation;
        transform.Rotation = Yogi::Quaternion::AngleAxis(kMouseSensitivity * (lastY - Yogi::Input::GetMouseY()),
                                                         transform.Rotation * Yogi::Vector3::Right()) *
            transform.Rotation;
        lastX = Yogi::Input::GetMouseX();
        lastY = Yogi::Input::GetMouseY();
        // transform.Rotation = Yogi::Quaternion::AngleAxis((float)ts * 20.0f, Yogi::Vector3::Up()) * transform.Rotation;
        m_world->OnUpdate(ts);
        // auto&                            swapChain     = Yogi::Application::GetInstance().GetSwapChain();
        // auto                             currentTarget = swapChain->GetCurrentTarget();
        // auto                             currentDepth  = swapChain->GetCurrentDepth();
        // Yogi::Owner<Yogi::IFrameBuffer> frameBuffer   = Yogi::IFrameBuffer::Create(Yogi::FrameBufferDesc{
        //     swapChain->GetWidth(),
        //     swapChain->GetHeight(),
        //     Yogi::WRef<Yogi::IRenderPass>::Create(m_renderPass),
        //       { currentTarget },
        //     currentDepth,
        // });

        // m_transform = Yogi::Matrix4::Rotation(0, (float)ts * 20.0f, 0) * m_transform;
        // Yogi::Matrix4 transform =
        //     Yogi::MathUtils::Perspective(
        //         45.0f, (float)swapChain->GetWidth() / (float)swapChain->GetHeight(), 0.1f, 100.0f) *
        //     Yogi::Matrix4::Translation(Yogi::Vector3{ 0, 0, -5 }) * m_transform;
        // m_uniformBuffer->UpdateData(&transform, sizeof(Yogi::Matrix4), 0);

        // Yogi::Owner<Yogi::ICommandBuffer> commandBuffer = Yogi::ICommandBuffer::Create(
        //     Yogi::CommandBufferDesc{ Yogi::CommandBufferUsage::OneTimeSubmit, Yogi::SubmitQueue::Graphics });
        // commandBuffer->Begin();
        // commandBuffer->BeginRenderPass(Yogi::WRef<Yogi::IFrameBuffer>::Create(frameBuffer),
        //                                { Yogi::ClearValue{ 0.1f, 0.1f, 0.1f, 1.0f } },
        //                                Yogi::ClearValue{ 1.0f, 0 });
        // commandBuffer->SetVertexBuffer(Yogi::WRef<Yogi::IBuffer>::Create(m_vertexBuffer));
        // commandBuffer->SetIndexBuffer(Yogi::WRef<Yogi::IBuffer>::Create(m_indexBuffer));
        // commandBuffer->SetPipeline(Yogi::WRef<Yogi::IPipeline>::Create(m_pipeline));
        // commandBuffer->SetShaderResourceBinding(
        //     Yogi::WRef<Yogi::IShaderResourceBinding>::Create(m_shaderResourceBinding));
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
    m_world->OnEvent(e);
}