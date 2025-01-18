
#include "smc.h"

#include "epi_sdl.h"
#include "nuklear_defines.h"
#include "sokol_gfx.h"
#include "sokol_nuklear.h"

extern "C"
{
    // defined in moon nuklear static lib
    int luaopen_moonnuklear(lua_State *L);
}

namespace smc
{

static void nk_sdl_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    const char *text = SDL_GetClipboardText();
    if (text)
        nk_textedit_paste(edit, text, nk_strlen(text));
    (void)usr;
}

static void nk_sdl_clipboard_copy(nk_handle usr, const char *text, int len)
{
    char *str = 0;
    (void)usr;
    if (!len)
        return;
    str = (char *)Mem_Alloc((size_t)len + 1);
    if (!str)
        return;
    memcpy(str, text, (size_t)len);
    str[len] = '\0';
    SDL_SetClipboardText(str);
    Mem_Free(str);
}

static int nk_sdl_handle_event(nk_context *ctx, SDL_Event *evt)
{
    switch (evt->type)
    {
    case SDL_KEYUP: /* KEYUP & KEYDOWN share same routine */
    case SDL_KEYDOWN: {
        int          down  = evt->type == SDL_KEYDOWN;
        const Uint8 *state = SDL_GetKeyboardState(0);
        switch (evt->key.keysym.sym)
        {
        case SDLK_RSHIFT: /* RSHIFT & LSHIFT share same routine */
        case SDLK_LSHIFT:
            nk_input_key(ctx, NK_KEY_SHIFT, down);
            break;
        case SDLK_DELETE:
            nk_input_key(ctx, NK_KEY_DEL, down);
            break;
        case SDLK_RETURN:
            nk_input_key(ctx, NK_KEY_ENTER, down);
            break;
        case SDLK_TAB:
            nk_input_key(ctx, NK_KEY_TAB, down);
            break;
        case SDLK_BACKSPACE:
            nk_input_key(ctx, NK_KEY_BACKSPACE, down);
            break;
        case SDLK_HOME:
            nk_input_key(ctx, NK_KEY_TEXT_START, down);
            nk_input_key(ctx, NK_KEY_SCROLL_START, down);
            break;
        case SDLK_END:
            nk_input_key(ctx, NK_KEY_TEXT_END, down);
            nk_input_key(ctx, NK_KEY_SCROLL_END, down);
            break;
        case SDLK_PAGEDOWN:
            nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
            break;
        case SDLK_PAGEUP:
            nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
            break;
        case SDLK_z:
            nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_r:
            nk_input_key(ctx, NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_c:
            nk_input_key(ctx, NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_v:
            nk_input_key(ctx, NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_x:
            nk_input_key(ctx, NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_b:
            nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_e:
            nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
            break;
        case SDLK_UP:
            nk_input_key(ctx, NK_KEY_UP, down);
            break;
        case SDLK_DOWN:
            nk_input_key(ctx, NK_KEY_DOWN, down);
            break;
        case SDLK_LEFT:
            if (state[SDL_SCANCODE_LCTRL])
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else
                nk_input_key(ctx, NK_KEY_LEFT, down);
            break;
        case SDLK_RIGHT:
            if (state[SDL_SCANCODE_LCTRL])
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else
                nk_input_key(ctx, NK_KEY_RIGHT, down);
            break;
        }
    }
        return 1;

    case SDL_MOUSEBUTTONUP: /* MOUSEBUTTONUP & MOUSEBUTTONDOWN share same routine */
    case SDL_MOUSEBUTTONDOWN: {
        int       down = evt->type == SDL_MOUSEBUTTONDOWN;
        const int x = evt->button.x, y = evt->button.y;
        switch (evt->button.button)
        {
        case SDL_BUTTON_LEFT:
            if (evt->button.clicks > 1)
                nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down);
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
            break;
        case SDL_BUTTON_MIDDLE:
            nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
            break;
        case SDL_BUTTON_RIGHT:
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
            break;
        }
    }
        return 1;

    case SDL_MOUSEMOTION:
        if (ctx->input.mouse.grabbed)
        {
            int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
            nk_input_motion(ctx, x + evt->motion.xrel, y + evt->motion.yrel);
        }
        else
            nk_input_motion(ctx, evt->motion.x, evt->motion.y);
        return 1;

    case SDL_TEXTINPUT: {
        nk_glyph glyph;
        memcpy(glyph, evt->text.text, NK_UTF_SIZE);
        nk_input_glyph(ctx, glyph);
    }
        return 1;

    case SDL_MOUSEWHEEL:
        nk_input_scroll(ctx, nk_vec2((float)evt->wheel.x, (float)evt->wheel.y));
        return 1;
    }
    return 0;
}

static epi::ILuaState *lua_state         = nullptr;
static nk_context     *global_nk_context = nullptr;

void SMC_InputBegin()
{
    if (!global_nk_context)
    {
        return;
    }

    nk_input_begin(global_nk_context);
}

void SMC_InputEnd()
{
    if (!global_nk_context)
    {
        return;
    }

    nk_input_end(global_nk_context);
}

int SMC_InputEvent(void *event)
{
    if (!global_nk_context)
    {
        return 0;
    }

    return nk_sdl_handle_event(global_nk_context, (SDL_Event *)event);
}

void SMC_Frame()
{
    // can happen when drawing intial stuff at boot
    if (!lua_state)
    {
        return;
    }

    nk_context *ctx = snk_new_frame();

    // todo: refactor this
    if (!global_nk_context)
    {

        ctx->clip.copy  = nk_sdl_clipboard_copy;
        ctx->clip.paste = nk_sdl_clipboard_paste;

        global_nk_context = ctx;
        lua_State *L      = lua_state->GetLuaState();
        int        top    = lua_gettop(L);
        lua_getglobal(L, "SMC_SetContext");
        lua_pushlightuserdata(L, (void *)ctx);
        lua_call(L, 1, 0);
        EPI_ASSERT(top == lua_gettop(L));
    }

    lua_state->CallGlobalFunction("SMC_Frame");

    snk_render(1920, 1080);
}

void SMC_Init(epi::LuaConfig &lua_config)
{
    EPI_ASSERT(!lua_state);

    snk_desc_t snk_desc = {0};
    snk_setup(&snk_desc);

    lua_config.modules_.push_back({luaopen_moonnuklear, "nk"});

    lua_state = epi::ILuaState::CreateVM(lua_config);

    lua_state->DoFile("scripts/lua/smc/test.lua");
}

} // namespace smc

#if !defined(SOKOL_NUKLEAR_NO_SOKOL_APP)
_SOKOL_PRIVATE bool _snk_is_ctrl(uint32_t modifiers)
{
    if (_snuklear.is_osx)
    {
        return 0 != (modifiers & SAPP_MODIFIER_SUPER);
    }
    else
    {
        return 0 != (modifiers & SAPP_MODIFIER_CTRL);
    }
}

_SOKOL_PRIVATE void _snk_append_char(uint32_t char_code)
{
    size_t idx = strlen(_snuklear.char_buffer);
    if (idx < NK_INPUT_MAX)
    {
        _snuklear.char_buffer[idx] = (char)char_code;
    }
}

_SOKOL_PRIVATE enum nk_keys _snk_event_to_nuklearkey(const sapp_event *ev)
{
    switch (ev->key_code)
    {
    case SAPP_KEYCODE_C:
        if (_snk_is_ctrl(ev->modifiers))
        {
            return NK_KEY_COPY;
        }
        else
        {
            return NK_KEY_NONE;
        }
        break;
    case SAPP_KEYCODE_X:
        if (_snk_is_ctrl(ev->modifiers))
        {
            return NK_KEY_CUT;
        }
        else
        {
            return NK_KEY_NONE;
        }
        break;
    case SAPP_KEYCODE_A:
        if (_snk_is_ctrl(ev->modifiers))
        {
            return NK_KEY_TEXT_SELECT_ALL;
        }
        else
        {
            return NK_KEY_NONE;
        }
        break;
    case SAPP_KEYCODE_Z:
        if (_snk_is_ctrl(ev->modifiers))
        {
            if (ev->modifiers & SAPP_MODIFIER_SHIFT)
            {
                return NK_KEY_TEXT_REDO;
            }
            else
            {
                return NK_KEY_TEXT_UNDO;
            }
        }
        else
        {
            return NK_KEY_NONE;
        }
        break;
    case SAPP_KEYCODE_DELETE:
        return NK_KEY_DEL;
    case SAPP_KEYCODE_ENTER:
        return NK_KEY_ENTER;
    case SAPP_KEYCODE_TAB:
        return NK_KEY_TAB;
    case SAPP_KEYCODE_BACKSPACE:
        return NK_KEY_BACKSPACE;
    case SAPP_KEYCODE_UP:
        return NK_KEY_UP;
    case SAPP_KEYCODE_DOWN:
        return NK_KEY_DOWN;
    case SAPP_KEYCODE_LEFT:
        return NK_KEY_LEFT;
    case SAPP_KEYCODE_RIGHT:
        return NK_KEY_RIGHT;
    case SAPP_KEYCODE_LEFT_SHIFT:
        return NK_KEY_SHIFT;
    case SAPP_KEYCODE_RIGHT_SHIFT:
        return NK_KEY_SHIFT;
    case SAPP_KEYCODE_LEFT_CONTROL:
        return NK_KEY_CTRL;
    case SAPP_KEYCODE_RIGHT_CONTROL:
        return NK_KEY_CTRL;
    default:
        return NK_KEY_NONE;
    }
}

SOKOL_API_IMPL bool snk_handle_event(const sapp_event *ev)
{
    SOKOL_ASSERT(_SNK_INIT_COOKIE == _snuklear.init_cookie);
    const float dpi_scale = _snuklear.desc.dpi_scale;
    switch (ev->type)
    {
    case SAPP_EVENTTYPE_MOUSE_DOWN:
        _snuklear.mouse_pos[0] = (int)(ev->mouse_x / dpi_scale);
        _snuklear.mouse_pos[1] = (int)(ev->mouse_y / dpi_scale);
        switch (ev->mouse_button)
        {
        case SAPP_MOUSEBUTTON_LEFT:
            _snuklear.btn_down[NK_BUTTON_LEFT] = true;
            break;
        case SAPP_MOUSEBUTTON_RIGHT:
            _snuklear.btn_down[NK_BUTTON_RIGHT] = true;
            break;
        case SAPP_MOUSEBUTTON_MIDDLE:
            _snuklear.btn_down[NK_BUTTON_MIDDLE] = true;
            break;
        default:
            break;
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_UP:
        _snuklear.mouse_pos[0] = (int)(ev->mouse_x / dpi_scale);
        _snuklear.mouse_pos[1] = (int)(ev->mouse_y / dpi_scale);
        switch (ev->mouse_button)
        {
        case SAPP_MOUSEBUTTON_LEFT:
            _snuklear.btn_up[NK_BUTTON_LEFT] = true;
            break;
        case SAPP_MOUSEBUTTON_RIGHT:
            _snuklear.btn_up[NK_BUTTON_RIGHT] = true;
            break;
        case SAPP_MOUSEBUTTON_MIDDLE:
            _snuklear.btn_up[NK_BUTTON_MIDDLE] = true;
            break;
        default:
            break;
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_MOVE:
        _snuklear.mouse_pos[0]   = (int)(ev->mouse_x / dpi_scale);
        _snuklear.mouse_pos[1]   = (int)(ev->mouse_y / dpi_scale);
        _snuklear.mouse_did_move = true;
        break;
    case SAPP_EVENTTYPE_MOUSE_ENTER:
    case SAPP_EVENTTYPE_MOUSE_LEAVE:
        for (int i = 0; i < NK_BUTTON_MAX; i++)
        {
            _snuklear.btn_down[i] = false;
            _snuklear.btn_up[i]   = false;
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        _snuklear.mouse_scroll[0]  = ev->scroll_x;
        _snuklear.mouse_scroll[1]  = ev->scroll_y;
        _snuklear.mouse_did_scroll = true;
        break;
    case SAPP_EVENTTYPE_TOUCHES_BEGAN:
        _snuklear.btn_down[NK_BUTTON_LEFT] = true;
        _snuklear.mouse_pos[0]             = (int)(ev->touches[0].pos_x / dpi_scale);
        _snuklear.mouse_pos[1]             = (int)(ev->touches[0].pos_y / dpi_scale);
        _snuklear.mouse_did_move           = true;
        break;
    case SAPP_EVENTTYPE_TOUCHES_MOVED:
        _snuklear.mouse_pos[0]   = (int)(ev->touches[0].pos_x / dpi_scale);
        _snuklear.mouse_pos[1]   = (int)(ev->touches[0].pos_y / dpi_scale);
        _snuklear.mouse_did_move = true;
        break;
    case SAPP_EVENTTYPE_TOUCHES_ENDED:
        _snuklear.btn_up[NK_BUTTON_LEFT] = true;
        _snuklear.mouse_pos[0]           = (int)(ev->touches[0].pos_x / dpi_scale);
        _snuklear.mouse_pos[1]           = (int)(ev->touches[0].pos_y / dpi_scale);
        _snuklear.mouse_did_move         = true;
        break;
    case SAPP_EVENTTYPE_TOUCHES_CANCELLED:
        _snuklear.btn_up[NK_BUTTON_LEFT]   = false;
        _snuklear.btn_down[NK_BUTTON_LEFT] = false;
        break;
    case SAPP_EVENTTYPE_KEY_DOWN:
        /* intercept Ctrl-V, this is handled via EVENTTYPE_CLIPBOARD_PASTED */
        if (_snk_is_ctrl(ev->modifiers) && (ev->key_code == SAPP_KEYCODE_V))
        {
            break;
        }
        /* on web platform, don't forward Ctrl-X, Ctrl-V to the browser */
        if (_snk_is_ctrl(ev->modifiers) && (ev->key_code == SAPP_KEYCODE_X))
        {
            sapp_consume_event();
        }
        if (_snk_is_ctrl(ev->modifiers) && (ev->key_code == SAPP_KEYCODE_C))
        {
            sapp_consume_event();
        }
        _snuklear.keys_down[_snk_event_to_nuklearkey(ev)] = true;
        break;
    case SAPP_EVENTTYPE_KEY_UP:
        /* intercept Ctrl-V, this is handled via EVENTTYPE_CLIPBOARD_PASTED */
        if (_snk_is_ctrl(ev->modifiers) && (ev->key_code == SAPP_KEYCODE_V))
        {
            break;
        }
        /* on web platform, don't forward Ctrl-X, Ctrl-V to the browser */
        if (_snk_is_ctrl(ev->modifiers) && (ev->key_code == SAPP_KEYCODE_X))
        {
            sapp_consume_event();
        }
        if (_snk_is_ctrl(ev->modifiers) && (ev->key_code == SAPP_KEYCODE_C))
        {
            sapp_consume_event();
        }
        _snuklear.keys_up[_snk_event_to_nuklearkey(ev)] = true;
        break;
    case SAPP_EVENTTYPE_CHAR:
        if ((ev->char_code >= 32) && (ev->char_code != 127) &&
            (0 == (ev->modifiers & (SAPP_MODIFIER_ALT | SAPP_MODIFIER_CTRL | SAPP_MODIFIER_SUPER))))
        {
            _snk_append_char(ev->char_code);
        }
        break;
    case SAPP_EVENTTYPE_CLIPBOARD_PASTED:
        _snuklear.keys_down[NK_KEY_PASTE] = _snuklear.keys_up[NK_KEY_PASTE] = true;
        break;
    default:
        break;
    }
    return nk_item_is_any_active(&_snuklear.ctx);
}

SOKOL_API_IMPL nk_flags snk_edit_string(struct nk_context *ctx, nk_flags flags, char *memory, int *len, int max,
                                        nk_plugin_filter filter)
{
    SOKOL_ASSERT(_SNK_INIT_COOKIE == _snuklear.init_cookie);
    nk_flags event = nk_edit_string(ctx, flags, memory, len, max, filter);
    if ((event & NK_EDIT_ACTIVATED) && !sapp_keyboard_shown())
    {
        sapp_show_keyboard(true);
    }
    if ((event & NK_EDIT_DEACTIVATED) && sapp_keyboard_shown())
    {
        sapp_show_keyboard(false);
    }
    return event;
}
#endif // SOKOL_NUKLEAR_NO_SOKOL_APP