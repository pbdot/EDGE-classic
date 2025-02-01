
#include "smc_host.h"

#include <epi.h>
#include <imgui.h>

#include "smc_public.h"

namespace edge
{

static bool smc_host_initialized = false;
static bool smc_host_active = false;

void SMC_Host_Initialize()
{
    if (smc_host_initialized)
    {
        FatalError("SMC_Host_Initialize: Attempting to reinitialize SMC Host");
    }

    smc_host_initialized = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    if (smc::SMC_Main())
    {
        FatalError("SMC_Host_Initialize: Failed to initialize\n");
    }
}

bool SMC_Host_Initialized()
{
    return smc_host_initialized;
}

bool SMC_Host_Activated()
{
    return smc_host_active;
}

void SMC_Host_Activate(bool activated)
{
    if (!SMC_Host_Initialized())    
    {
        SMC_Host_Initialize();
    }

    smc_host_active = activated;
}

void SMC_Host_Deactivate()
{
    SMC_Host_Activate(false);
}

void SMC_Host_Shutdown()
{
    if (!smc_host_initialized)
    {
        return;
    }

    smc::SMC_Shutdown();
}

ImGuiKey ImGui_ImplSDL2_KeyEventToImGuiKey(SDL_Keycode keycode, SDL_Scancode scancode)
{
    IM_UNUSED(scancode);
    switch (keycode)
    {
    case SDLK_TAB:
        return ImGuiKey_Tab;
    case SDLK_LEFT:
        return ImGuiKey_LeftArrow;
    case SDLK_RIGHT:
        return ImGuiKey_RightArrow;
    case SDLK_UP:
        return ImGuiKey_UpArrow;
    case SDLK_DOWN:
        return ImGuiKey_DownArrow;
    case SDLK_PAGEUP:
        return ImGuiKey_PageUp;
    case SDLK_PAGEDOWN:
        return ImGuiKey_PageDown;
    case SDLK_HOME:
        return ImGuiKey_Home;
    case SDLK_END:
        return ImGuiKey_End;
    case SDLK_INSERT:
        return ImGuiKey_Insert;
    case SDLK_DELETE:
        return ImGuiKey_Delete;
    case SDLK_BACKSPACE:
        return ImGuiKey_Backspace;
    case SDLK_SPACE:
        return ImGuiKey_Space;
    case SDLK_RETURN:
        return ImGuiKey_Enter;
    case SDLK_ESCAPE:
        return ImGuiKey_Escape;
    case SDLK_QUOTE:
        return ImGuiKey_Apostrophe;
    case SDLK_COMMA:
        return ImGuiKey_Comma;
    case SDLK_MINUS:
        return ImGuiKey_Minus;
    case SDLK_PERIOD:
        return ImGuiKey_Period;
    case SDLK_SLASH:
        return ImGuiKey_Slash;
    case SDLK_SEMICOLON:
        return ImGuiKey_Semicolon;
    case SDLK_EQUALS:
        return ImGuiKey_Equal;
    case SDLK_LEFTBRACKET:
        return ImGuiKey_LeftBracket;
    case SDLK_BACKSLASH:
        return ImGuiKey_Backslash;
    case SDLK_RIGHTBRACKET:
        return ImGuiKey_RightBracket;
    case SDLK_BACKQUOTE:
        return ImGuiKey_GraveAccent;
    case SDLK_CAPSLOCK:
        return ImGuiKey_CapsLock;
    case SDLK_SCROLLLOCK:
        return ImGuiKey_ScrollLock;
    case SDLK_NUMLOCKCLEAR:
        return ImGuiKey_NumLock;
    case SDLK_PRINTSCREEN:
        return ImGuiKey_PrintScreen;
    case SDLK_PAUSE:
        return ImGuiKey_Pause;
    case SDLK_KP_0:
        return ImGuiKey_Keypad0;
    case SDLK_KP_1:
        return ImGuiKey_Keypad1;
    case SDLK_KP_2:
        return ImGuiKey_Keypad2;
    case SDLK_KP_3:
        return ImGuiKey_Keypad3;
    case SDLK_KP_4:
        return ImGuiKey_Keypad4;
    case SDLK_KP_5:
        return ImGuiKey_Keypad5;
    case SDLK_KP_6:
        return ImGuiKey_Keypad6;
    case SDLK_KP_7:
        return ImGuiKey_Keypad7;
    case SDLK_KP_8:
        return ImGuiKey_Keypad8;
    case SDLK_KP_9:
        return ImGuiKey_Keypad9;
    case SDLK_KP_PERIOD:
        return ImGuiKey_KeypadDecimal;
    case SDLK_KP_DIVIDE:
        return ImGuiKey_KeypadDivide;
    case SDLK_KP_MULTIPLY:
        return ImGuiKey_KeypadMultiply;
    case SDLK_KP_MINUS:
        return ImGuiKey_KeypadSubtract;
    case SDLK_KP_PLUS:
        return ImGuiKey_KeypadAdd;
    case SDLK_KP_ENTER:
        return ImGuiKey_KeypadEnter;
    case SDLK_KP_EQUALS:
        return ImGuiKey_KeypadEqual;
    case SDLK_LCTRL:
        return ImGuiKey_LeftCtrl;
    case SDLK_LSHIFT:
        return ImGuiKey_LeftShift;
    case SDLK_LALT:
        return ImGuiKey_LeftAlt;
    case SDLK_LGUI:
        return ImGuiKey_LeftSuper;
    case SDLK_RCTRL:
        return ImGuiKey_RightCtrl;
    case SDLK_RSHIFT:
        return ImGuiKey_RightShift;
    case SDLK_RALT:
        return ImGuiKey_RightAlt;
    case SDLK_RGUI:
        return ImGuiKey_RightSuper;
    case SDLK_APPLICATION:
        return ImGuiKey_Menu;
    case SDLK_0:
        return ImGuiKey_0;
    case SDLK_1:
        return ImGuiKey_1;
    case SDLK_2:
        return ImGuiKey_2;
    case SDLK_3:
        return ImGuiKey_3;
    case SDLK_4:
        return ImGuiKey_4;
    case SDLK_5:
        return ImGuiKey_5;
    case SDLK_6:
        return ImGuiKey_6;
    case SDLK_7:
        return ImGuiKey_7;
    case SDLK_8:
        return ImGuiKey_8;
    case SDLK_9:
        return ImGuiKey_9;
    case SDLK_a:
        return ImGuiKey_A;
    case SDLK_b:
        return ImGuiKey_B;
    case SDLK_c:
        return ImGuiKey_C;
    case SDLK_d:
        return ImGuiKey_D;
    case SDLK_e:
        return ImGuiKey_E;
    case SDLK_f:
        return ImGuiKey_F;
    case SDLK_g:
        return ImGuiKey_G;
    case SDLK_h:
        return ImGuiKey_H;
    case SDLK_i:
        return ImGuiKey_I;
    case SDLK_j:
        return ImGuiKey_J;
    case SDLK_k:
        return ImGuiKey_K;
    case SDLK_l:
        return ImGuiKey_L;
    case SDLK_m:
        return ImGuiKey_M;
    case SDLK_n:
        return ImGuiKey_N;
    case SDLK_o:
        return ImGuiKey_O;
    case SDLK_p:
        return ImGuiKey_P;
    case SDLK_q:
        return ImGuiKey_Q;
    case SDLK_r:
        return ImGuiKey_R;
    case SDLK_s:
        return ImGuiKey_S;
    case SDLK_t:
        return ImGuiKey_T;
    case SDLK_u:
        return ImGuiKey_U;
    case SDLK_v:
        return ImGuiKey_V;
    case SDLK_w:
        return ImGuiKey_W;
    case SDLK_x:
        return ImGuiKey_X;
    case SDLK_y:
        return ImGuiKey_Y;
    case SDLK_z:
        return ImGuiKey_Z;
    case SDLK_F1:
        return ImGuiKey_F1;
    case SDLK_F2:
        return ImGuiKey_F2;
    case SDLK_F3:
        return ImGuiKey_F3;
    case SDLK_F4:
        return ImGuiKey_F4;
    case SDLK_F5:
        return ImGuiKey_F5;
    case SDLK_F6:
        return ImGuiKey_F6;
    case SDLK_F7:
        return ImGuiKey_F7;
    case SDLK_F8:
        return ImGuiKey_F8;
    case SDLK_F9:
        return ImGuiKey_F9;
    case SDLK_F10:
        return ImGuiKey_F10;
    case SDLK_F11:
        return ImGuiKey_F11;
    case SDLK_F12:
        return ImGuiKey_F12;
    case SDLK_F13:
        return ImGuiKey_F13;
    case SDLK_F14:
        return ImGuiKey_F14;
    case SDLK_F15:
        return ImGuiKey_F15;
    case SDLK_F16:
        return ImGuiKey_F16;
    case SDLK_F17:
        return ImGuiKey_F17;
    case SDLK_F18:
        return ImGuiKey_F18;
    case SDLK_F19:
        return ImGuiKey_F19;
    case SDLK_F20:
        return ImGuiKey_F20;
    case SDLK_F21:
        return ImGuiKey_F21;
    case SDLK_F22:
        return ImGuiKey_F22;
    case SDLK_F23:
        return ImGuiKey_F23;
    case SDLK_F24:
        return ImGuiKey_F24;
    case SDLK_AC_BACK:
        return ImGuiKey_AppBack;
    case SDLK_AC_FORWARD:
        return ImGuiKey_AppForward;
    default:
        break;
    }
    return ImGuiKey_None;
}

static void ImGui_ImplSDL2_UpdateKeyModifiers(SDL_Keymod sdl_key_mods)
{
    ImGuiIO &io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, (sdl_key_mods & KMOD_CTRL) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (sdl_key_mods & KMOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (sdl_key_mods & KMOD_ALT) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (sdl_key_mods & KMOD_GUI) != 0);
}

static ImGuiViewport *SMC_Host_GetViewportForWindowID(Uint32 window_id)
{
    EPI_UNUSED(window_id);

    // TODO: multiple top level edit windows

    return ImGui::GetMainViewport();
    // return ImGui::FindViewportByPlatformHandle((void*)(intptr_t)window_id);
}

bool SMC_Host_HandleEvent(const SDL_Event *event)
{
    if (!smc_host_active)
    {
        return false;
    }

    ImGuiIO &io = ImGui::GetIO();

    switch (event->type)
    {
    case SDL_MOUSEMOTION: {
        if (SMC_Host_GetViewportForWindowID(event->motion.windowID) == nullptr)
            return false;
        ImVec2 mouse_pos((float)event->motion.x, (float)event->motion.y);
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            int window_x, window_y;
            SDL_GetWindowPosition(SDL_GetWindowFromID(event->motion.windowID), &window_x, &window_y);
            mouse_pos.x += window_x;
            mouse_pos.y += window_y;
        }
        io.AddMouseSourceEvent(event->motion.which == SDL_TOUCH_MOUSEID ? ImGuiMouseSource_TouchScreen
                                                                        : ImGuiMouseSource_Mouse);
        io.AddMousePosEvent(mouse_pos.x, mouse_pos.y);
        return true;
    }
    case SDL_MOUSEWHEEL: {
        if (SMC_Host_GetViewportForWindowID(event->wheel.windowID) == nullptr)
            return false;
        // IMGUI_DEBUG_LOG("wheel %.2f %.2f, precise %.2f %.2f\n", (float)event->wheel.x, (float)event->wheel.y,
        // event->wheel.preciseX, event->wheel.preciseY);
#if SDL_VERSION_ATLEAST(2, 0, 18) // If this fails to compile on Emscripten: update to latest Emscripten!
        float wheel_x = -event->wheel.preciseX;
        float wheel_y = event->wheel.preciseY;
#else
        float wheel_x = -(float)event->wheel.x;
        float wheel_y = (float)event->wheel.y;
#endif
#if defined(__EMSCRIPTEN__) && !SDL_VERSION_ATLEAST(2, 31, 0)
        wheel_x /= 100.0f;
#endif
        io.AddMouseSourceEvent(event->wheel.which == SDL_TOUCH_MOUSEID ? ImGuiMouseSource_TouchScreen
                                                                       : ImGuiMouseSource_Mouse);
        io.AddMouseWheelEvent(wheel_x, wheel_y);
        return true;
    }
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        if (SMC_Host_GetViewportForWindowID(event->button.windowID) == nullptr)
            return false;
        int mouse_button = -1;
        if (event->button.button == SDL_BUTTON_LEFT)
        {
            mouse_button = 0;
        }
        if (event->button.button == SDL_BUTTON_RIGHT)
        {
            mouse_button = 1;
        }
        if (event->button.button == SDL_BUTTON_MIDDLE)
        {
            mouse_button = 2;
        }
        if (event->button.button == SDL_BUTTON_X1)
        {
            mouse_button = 3;
        }
        if (event->button.button == SDL_BUTTON_X2)
        {
            mouse_button = 4;
        }
        if (mouse_button == -1)
            break;
        io.AddMouseSourceEvent(event->button.which == SDL_TOUCH_MOUSEID ? ImGuiMouseSource_TouchScreen
                                                                        : ImGuiMouseSource_Mouse);
        io.AddMouseButtonEvent(mouse_button, (event->type == SDL_MOUSEBUTTONDOWN));
        // FIXME
        /*
        bd->MouseButtonsDown = (event->type == SDL_MOUSEBUTTONDOWN) ? (bd->MouseButtonsDown | (1 << mouse_button))
                                                                    : (bd->MouseButtonsDown & ~(1 << mouse_button));
        */
        return true;
    }
    case SDL_TEXTINPUT: {
        if (SMC_Host_GetViewportForWindowID(event->text.windowID) == nullptr)
            return false;
        io.AddInputCharactersUTF8(event->text.text);
        return true;
    }
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        if (SMC_Host_GetViewportForWindowID(event->key.windowID) == nullptr)
            return false;
        ImGui_ImplSDL2_UpdateKeyModifiers((SDL_Keymod)event->key.keysym.mod);
        // IMGUI_DEBUG_LOG("SDL_KEY_%s : key=%d ('%s'), scancode=%d ('%s'), mod=%X\n",
        //     (event->type == SDL_KEYDOWN) ? "DOWN" : "UP  ", event->key.keysym.sym,
        //     SDL_GetKeyName(event->key.keysym.sym), event->key.keysym.scancode,
        //     SDL_GetScancodeName(event->key.keysym.scancode), event->key.keysym.mod);
        ImGuiKey key = ImGui_ImplSDL2_KeyEventToImGuiKey(event->key.keysym.sym, event->key.keysym.scancode);
        io.AddKeyEvent(key, (event->type == SDL_KEYDOWN));
        io.SetKeyEventNativeData(
            key, event->key.keysym.sym, event->key.keysym.scancode,
            event->key.keysym.scancode); // To support legacy indexing (<1.87 user code). Legacy backend uses SDLK_***
                                         // as indices to IsKeyXXX() functions.

        // TODO: Fix me, this is also in the edge input code for dev purposes
        if (event->type == SDL_KEYUP)
        {
            if (key == ImGuiKey_Keypad0)
            {
                SMC_Host_Activate(false);
            }
        }

        return true;
    }
#if SDL_HAS_DISPLAY_EVENT
    case SDL_DISPLAYEVENT: {
        // 2.0.26 has SDL_DISPLAYEVENT_CONNECTED/SDL_DISPLAYEVENT_DISCONNECTED/SDL_DISPLAYEVENT_ORIENTATION,
        // so change of DPI/Scaling are not reflected in this event. (SDL3 has it)
        bd->WantUpdateMonitors = true;
        return true;
    }
#endif
    case SDL_WINDOWEVENT: {
        ImGuiViewport *viewport = SMC_Host_GetViewportForWindowID(event->window.windowID);
        if (viewport == NULL)
            return false;

        // - When capturing mouse, SDL will send a bunch of conflicting LEAVE/ENTER event on every mouse move, but the
        // final ENTER tends to be right.
        // - However we won't get a correct LEAVE event for a captured window.
        // - In some cases, when detaching a window from main viewport SDL may send SDL_WINDOWEVENT_ENTER one frame too
        // late,
        //   causing SDL_WINDOWEVENT_LEAVE on previous frame to interrupt drag operation by clear mouse position. This
        //   is why we delay process the SDL_WINDOWEVENT_LEAVE events by one frame. See issue #5012 for details.
        Uint8 window_event = event->window.event;
        if (window_event == SDL_WINDOWEVENT_ENTER)
        {
            // FIXME
            /*
            bd->MouseWindowID       = event->window.windowID;
            bd->MouseLastLeaveFrame = 0;
            */
        }

        // FIXME
        /*
        if (window_event == SDL_WINDOWEVENT_LEAVE)
            bd->MouseLastLeaveFrame = ImGui::GetFrameCount() + 1;
        */

        if (window_event == SDL_WINDOWEVENT_FOCUS_GAINED)
            io.AddFocusEvent(true);
        else if (window_event == SDL_WINDOWEVENT_FOCUS_LOST)
            io.AddFocusEvent(false);
        else if (window_event == SDL_WINDOWEVENT_CLOSE)
            viewport->PlatformRequestClose = true;
        else if (window_event == SDL_WINDOWEVENT_MOVED)
            viewport->PlatformRequestMove = true;
        else if (window_event == SDL_WINDOWEVENT_RESIZED)
            viewport->PlatformRequestResize = true;
        return true;
    }
    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED: {
        // FIXME
        /*
        bd->WantUpdateGamepadsList = true;
        */
        return true;
    }
    }
    return false;
}

void SMC_Host_StartFrame()
{
    if (!SMC_Host_Activated())
    {
        return;
    }

    smc::SMC_ImGui_StartFrame();
}

void SMC_Host_FinishFrame()
{
    if (!SMC_Host_Activated())
    {
        return;
    }

    smc::SMC_ImGui_FinishFrame();

}


} // namespace edge
