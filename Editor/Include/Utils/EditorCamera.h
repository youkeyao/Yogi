#pragma once

#include <Yogi.h>

namespace Yogi
{

class EditorCamera
{
public:
    EditorCamera();

    const TransformComponent& GetTransformComponent() const { return m_transformComponent; }
    const CameraComponent&    GetCameraComponent() const { return m_cameraComponent; }

    void SetTransformComponent(const TransformComponent& transform) { m_transformComponent = transform; }
    void SetCameraComponent(const CameraComponent& camera) { m_cameraComponent = camera; }

private:
    TransformComponent m_transformComponent;
    CameraComponent    m_cameraComponent;
};

} // namespace Yogi
