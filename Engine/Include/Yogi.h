#pragma once

// ---core-----------------------------------
#include "Core/Application.h"
#include "Core/Input.h"
#include "Core/Layer.h"
#include "Core/Timestep.h"
#include "Events/ApplicationEvent.h"
#include "Events/Event.h"
#include "Events/KeyCodes.h"
#include "Events/KeyEvent.h"
#include "Events/MouseButtonCodes.h"
#include "Events/MouseEvent.h"
// --------------------------------------------

// ---math---------------------------------------
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"
#include "Math/MathUtils.h"
// ----------------------------------------------

// ---renderer----------------------------------
#include "Renderer/RHI/IBuffer.h"
#include "Renderer/RHI/IFrameBuffer.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/IRenderPass.h"
#include "Renderer/RHI/IShaderResourceBinding.h"

#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Renderer/RenderComponents.h"
#include "Renderer/ForwardRenderSystem.h"
// ---------------------------------------------

// ---scene------------------------------------
#include "Scene/World.h"
#include "Scene/Entity.h"
#include "Scene/ComponentBase.h"
// ---------------------------------------------

// ---resources-------------------------------
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/AssetManager/FileSystemSource.h"
#include "Resources/ResourceManager/ResourceManager.h"
// ---------------------------------------------