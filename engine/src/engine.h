#pragma once

#include "base/core/application.h"
#include "base/core/input.h"
#include "base/core/key_codes.h"
#include "base/core/mouse_button_codes.h"
#include "base/core/layer.h"
#include "base/core/timestep.h"

#include "base/events/event.h"
#include "base/events/application_event.h"
#include "base/events/key_event.h"
#include "base/events/mouse_event.h"

// ---renderer----------------------------------
#include "base/renderer/renderer.h"
#include "base/renderer/renderer_2d.h"
#include "base/renderer/renderer_command.h"

#include "base/renderer/buffer.h"
#include "base/renderer/shader.h"
#include "base/renderer/shader_library.h"
#include "base/renderer/texture.h"
#include "base/renderer/vertex_array.h"

#include "base/renderer/orthographic_camera.h"
#include "base/renderer/orthographic_camera_controller.h"
// ---------------------------------------------