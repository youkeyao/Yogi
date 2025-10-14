#pragma once

#include <Yogi.h>

namespace Yogi
{

class EditorCamera
{
public:
    EditorCamera();

    TransformComponent& GetTransformComponent() { return m_transformComponent; }
    CameraComponent&    GetCameraComponent() { return m_cameraComponent; }

private:
    TransformComponent m_transformComponent;
    CameraComponent    m_cameraComponent;
};

} // namespace Yogi
