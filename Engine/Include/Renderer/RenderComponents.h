#pragma once

#include "Math/MathUtils.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Resources/AssetManager.h"

namespace Yogi
{

struct CameraComponent
{
    float Fov         = MathUtils::ToRadians(45.0f);
    float AspectRatio = 1.0f;
    float ZoomLevel   = 1.0f;
    bool  IsOrtho     = true;
};

struct MeshRendererComponent
{
    AssetHandle<Mesh>     Mesh;
    AssetHandle<Material> Material;
    bool                  CastShadow = true;
};

} // namespace Yogi
