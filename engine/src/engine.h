#pragma once

#include "runtime/core/application.h"
#include "runtime/core/input.h"
#include "runtime/core/layer.h"
#include "runtime/core/timestep.h"

#include "runtime/events/event.h"
#include "runtime/events/application_event.h"
#include "runtime/events/key_event.h"
#include "runtime/events/mouse_event.h"
#include "runtime/events/key_codes.h"
#include "runtime/events/mouse_button_codes.h"

#include "runtime/scene/scene.h"
#include "runtime/scene/entity.h"
#include "runtime/scene/components.h"

#include "runtime/systems/render_system.h"
#include "runtime/systems/camera_system.h"

// ---renderer----------------------------------
#include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"

#include "runtime/renderer/buffer.h"
#include "runtime/renderer/frame_buffer.h"
#include "runtime/renderer/shader.h"
#include "runtime/renderer/texture.h"
// ---------------------------------------------