#pragma once

#include "Math/MathUtils.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Renderer/RHI/ITextureView.h"

namespace Yogi
{

struct CameraComponent
{
    WRef<ITextureView> Target    = nullptr;
    float              Fov       = 45.0f;
    float              ZoomLevel = 1.0f;
    bool               IsOrtho   = false;
};

struct MeshRendererComponent
{
    WRef<Mesh>     Mesh;
    WRef<Material> Material;
    bool           CastShadow = true;
};

} // namespace Yogi
