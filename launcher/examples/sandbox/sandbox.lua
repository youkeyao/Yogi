function init_on_update(ts, scene)
    local quad = MeshManager.get_mesh("quad");
    local mat = MaterialManager.get_material("default");

    local checker = scene:create_entity()
    checker:add_component("Yogi::TagComponent").tag = "1"
    checker:add_component("Yogi::TransformComponent").transform = Transform.translate(vec3(0, -0.3, 0))
    local mesh_renderer = checker:add_component("Yogi::MeshRendererComponent")
    mesh_renderer.mesh = quad
    mesh_renderer.material = mat
    checker:add_component("RotateComponent")

    local e = scene:create_entity()
    e:add_component("Yogi::TagComponent").tag = "2"
    e:add_component("Yogi::TransformComponent").transform = Transform.translate(vec3(0, 0, 0.05))
    mesh_renderer = e:add_component("Yogi::MeshRendererComponent")
    mesh_renderer.mesh = quad
    mesh_renderer.material = mat

    e = scene:create_entity()
    e:add_component("Yogi::TagComponent").tag = "3"
    e:add_component("Yogi::TransformComponent").transform = Transform.translate(vec3(0.4, 0, 0.1))
    mesh_renderer = e:add_component("Yogi::MeshRendererComponent")
    mesh_renderer.mesh = quad
    mesh_renderer.material = mat

    e = scene:create_entity()
    e:add_component("Yogi::TagComponent").tag = "camera"
    e:add_component("Yogi::TransformComponent").transform = Transform.lookat(vec3(2, 1, 2), vec3(0, 0, 0), vec3(0, 1, 0)):inverse()
    local cam = e:add_component("Yogi::CameraComponent")
    cam.aspect_ratio = 1280 / 720
    cam.is_ortho = false

    for i = 1, 100 do
        for j = 1, 100 do
            e = scene:create_entity()
            e:add_component("Yogi::TagComponent").tag = "quad"
            e:add_component("Yogi::TransformComponent").transform = Transform.translate(vec3(0.02 * i - 1, 0.02 * j - 1, 0.11)) * Transform.scale(vec3(0.01, 0.01, 0.01))
            mesh_renderer = e:add_component("Yogi::MeshRendererComponent")
            mesh_renderer.mesh = quad
            mesh_renderer.material = mat
        end
    end

    e = scene:create_entity()
    e:add_component("Yogi::TagComponent").tag = "pointlight"
    e:add_component("Yogi::TransformComponent").transform = Transform.translate(vec3(0, 0, 1))
    e:add_component("Yogi::PointLightComponent")

    e = scene:create_entity()
    e:add_component("Yogi::TagComponent").tag = "light"
    e:add_component("Yogi::TransformComponent").transform = Transform.translate(vec3(0, 0, 5))
    e:add_component("Yogi::DirectionalLightComponent")

    e = scene:create_entity()
    e:add_component("Yogi::TagComponent").tag = "skybox"
    e:add_component("Yogi::TransformComponent")
    e:add_component("Yogi::SkyboxComponent")

    e = scene:create_entity()
    e:add_component("Yogi::TagComponent").tag = "cube1"
    e:add_component("Yogi::TransformComponent").transform = Transform.translate(vec3(0, 0.5, 1)) * Transform.scale(vec3(0.5, 0.5, 0.5))
    mesh_renderer = e:add_component("Yogi::MeshRendererComponent")
    mesh_renderer.mesh = MeshManager.get_mesh("cube")
    mesh_renderer.material = mat
    e:add_component("Yogi::RigidBodyComponent")

    e = scene:create_entity()
    e:add_component("Yogi::TagComponent").tag = "cube2"
    e:add_component("Yogi::TransformComponent").transform = Transform.rotate(-10, vec3(1, 0, 0)) * Transform.translate(vec3(0, -1, 0)) * Transform.scale(vec3(4, 1, 4))
    mesh_renderer = e:add_component("Yogi::MeshRendererComponent")
    mesh_renderer.mesh = MeshManager.get_mesh("cube")
    mesh_renderer.material = mat
    e:add_component("Yogi::RigidBodyComponent").is_static = true

    scene:remove_system("InitSystem")
end

function init_on_event(event, scene)
end

function rotate_on_update(ts, scene)
    scene:view_components({"RotateComponent"}, function(e)
        local rotate = e:get_component("RotateComponent")
        local transform = e:get_component("Yogi::TransformComponent")
        transform.transform = Transform.rotate(ts:get_seconds() * 180 / 3.1415926, vec3(0, 1, 0)) * transform.transform
    end)
    print(1 / ts:get_seconds())
end

function rotate_on_event(event, scene)
end

register_system("InitSystem", init_on_update, init_on_event)
register_system("RotateSystem", rotate_on_update, rotate_on_event)
register_component("RotateComponent", { speed = "float" })