
#include "ui_backend.h"
#include "ui_platform.h"
#include "ui_renderer.h"

static SystemInterface_SDL *system_interface = nullptr;
static RenderInterface_GL2 *render_interface = nullptr;

// Initializes the backend, including the custom system and render interfaces,
// and opens a window for rendering the RmlUi context.
bool Backend::Initialize(const char *window_name, int width, int height,
                         bool allow_resize) {
  render_interface = new RenderInterface_GL2();
  system_interface = new SystemInterface_SDL();

  render_interface->SetViewport(width, height);
  return true;
}
// Closes the window and release all resources owned by the backend, including
// the system and render interfaces.
void Backend::Shutdown() {
  if (system_interface) {
    delete system_interface;
  }

  if (render_interface) {
    delete render_interface;
  }

  system_interface = nullptr;
  render_interface = nullptr;
}

// Returns a pointer to the custom system interface which should be provided to
// RmlUi.
Rml::SystemInterface *Backend::GetSystemInterface() { return system_interface; }

// Returns a pointer to the custom render interface which should be provided to
// RmlUi.
Rml::RenderInterface *Backend::GetRenderInterface() { return render_interface; }

// Polls and processes events from the current platform, and applies any
// relevant events to the provided RmlUi context and the key down callback.
// @return False to indicate that the application should be closed.
bool Backend::ProcessEvents(Rml::Context *context,
                            KeyDownCallback key_down_callback, bool power_save) {
  return true;
}

bool Backend::ProcessEvent(Rml::Context *context, SDL_Event &event,
                           KeyDownCallback key_down_callback) {
  switch (event.type) {
  case SDL_QUIT:
    break;
  case SDL_KEYDOWN: {
    const Rml::Input::KeyIdentifier key =
        RmlSDL::ConvertKey(event.key.keysym.sym);
    const int key_modifier = RmlSDL::GetKeyModifierState();
    const float native_dp_ratio = 1.f;

    // See if we have any global shortcuts that take priority over the context.
    if (key_down_callback &&
        !key_down_callback(context, key, key_modifier, native_dp_ratio, true))
      break;
    // Otherwise, hand the event over to the context by calling the input
    // handler as normal.
    if (!RmlSDL::InputEventHandler(context, event))
      break;
    // The key was not consumed by the context either, try keyboard shortcuts of
    // lower priority.
    if (key_down_callback &&
        !key_down_callback(context, key, key_modifier, native_dp_ratio, false))
      break;
  } break;
  case SDL_WINDOWEVENT: {
    switch (event.window.event) {
    case SDL_WINDOWEVENT_SIZE_CHANGED: {
      Rml::Vector2i dimensions(event.window.data1, event.window.data2);
      render_interface->SetViewport(dimensions.x, dimensions.y);
    } break;
    }
    RmlSDL::InputEventHandler(context, event);
  } break;
  default: {
    RmlSDL::InputEventHandler(context, event);
  } break;
  }

  return true;
}

// Request application closure during the next event processing call.
void Backend::RequestExit() {}

// Prepares the render state to accept rendering commands from RmlUi, call
// before rendering the RmlUi context.
void Backend::BeginFrame() {
  if (!render_interface) {
    return;
  }

  render_interface->BeginFrame();
}

// Presents the rendered frame to the screen, call after rendering the RmlUi
// context.
void Backend::PresentFrame() {
  if (!render_interface) {
    return;
  }

  render_interface->EndFrame();
}
