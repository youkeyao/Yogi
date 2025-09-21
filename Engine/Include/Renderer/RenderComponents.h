#pragma once

#include "Math/MathUtils.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"

namespace Yogi
{

struct CameraComponent
{
    float Fov       = 45.0f;
    float ZoomLevel = 1.0f;
    bool  IsOrtho   = false;
};

struct MeshRendererComponent
{
    Ref<Mesh>     Mesh;
    Ref<Material> Material;
    bool          CastShadow = true;
};

} // namespace Yogi
