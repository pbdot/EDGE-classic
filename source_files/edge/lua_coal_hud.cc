

#include "i_defs.h"
#include "main.h"
#include "font.h"
#include "image_data.h"
#include "dm_state.h"
#include "e_main.h"
#include "g_game.h"
#include "w_wad.h"
#include "e_player.h"
#include "hu_font.h"
#include "hu_draw.h"
#include "r_misc.h"
#include "r_modes.h"
#include "am_map.h" // AM_Drawer
#include "r_colormap.h"
#include "s_sound.h"
#include "rad_trig.h" //Lobo: need this to access RTS
#include "lua_edge.h"

#include <cmath>

extern std::string w_map_title;
extern cvar_c r_doubleframes;
extern bool erraticism_active;

extern player_t* lua_ui_player_who;
static player_t *ui_hud_who = NULL;


static int ui_hud_automap_flags[2]; // 0 = disabled, 1 = enabled
static float ui_hud_automap_zoom;

// Needed for color functions
extern epi::image_data_c* ReadAsEpiBlock(image_c* rim);

extern epi::image_data_c*
R_PalettisedToRGB(epi::image_data_c* src, const byte* palette, int opacity);

static rgbcol_t Vec3ToColor(const luabridge::LuaRef& v)
{
    SYS_ASSERT(v.isTable());

    const double vr = v[1].cast<double>().value();
    const double vg = v[2].cast<double>().value();
    const double vb = v[3].cast<double>().value();

    if (vr < 0 || vg < 0 || vb < 0)
        return RGB_NO_VALUE;

    int r = CLAMP(0, (int) vr, 255);
    int g = CLAMP(0, (int) vg, 255);
    int b = CLAMP(0, (int) vb, 255);

    rgbcol_t rgb = RGB_MAKE(r, g, b);

    // ensure we don't get the "no color" value by mistake
    if (rgb == RGB_NO_VALUE)
        rgb ^= 0x000101;

    return rgb;
}

static lua_State* hud_state = nullptr;

void LUA_Coal_OpenHud(lua_State* state)
{
    hud_state = state;

    luabridge::getGlobalNamespace(state)
        .beginNamespace("coal")
        .beginNamespace("hud")
        .addProperty(
            "custom_stbar", +[] { return W_IsLumpInPwad("STBAR") ? 1 : 0; })
        // query functions
        .addFunction(
            "game_mode",
            +[] {
                if (DEATHMATCH())
                    return "dm";
                else if (COOP_MATCH())
                    return "coop";
                else
                    return "sp";
            })
        .addFunction(
            "game_name",
            +[] {
                gamedef_c* g = currmap->episode;
                SYS_ASSERT(g);
                return g->name.c_str();
            })
        .addFunction(
            "map_name", +[] { return currmap->name.c_str(); })
        .addFunction(
            "map_title", +[] { return w_map_title.c_str(); })
        .addFunction(
            "map_author", +[] { return currmap->author.c_str(); })
        .addFunction(
            "which_hud", +[] { return screen_hud; })
        .addFunction(
            "check_automap", +[] { return automapactive ? 1 : 0; })
        .addFunction(
            "get_time", +[] { return I_GetTime() / (r_doubleframes.d ? 2 : 1); })
        // set-state functions
        .addFunction("coord_sys",
                     [state](int w, int h) {
                         if (w < 64 || h < 64)
                             I_Error("Bad hud.coord_sys size: %dx%d\n", w, h);

                         HUD_SetCoordSys(w, h);

                         luabridge::LuaRef coal = luabridge::getGlobal(state, "coal");
                         coal["hud"]["x_left"] = hud_x_left;
                         coal["hud"]["x_right"] = hud_x_right;
                     })
        .addFunction(
            "text_font",
            +[](const char* font_name) {
                fontdef_c* DEF = fontdefs.Lookup(font_name);
                SYS_ASSERT(DEF);

                if (!DEF)
                    I_Error("hud.text_font: Bad font name: %s\n", font_name);

                font_c* font = hu_fonts.Lookup(DEF);
                SYS_ASSERT(font);

                if (!font)
                    I_Error("hud.text_font: Bad font name: %s\n", font_name);

                HUD_SetFont(font);
            })
        .addFunction(
            "text_color", +[](luabridge::LuaRef v) { HUD_SetTextColor(Vec3ToColor(v)); })
        .addFunction(
            "set_scale",
            +[](double scale) {
                if (scale <= 0)
                    I_Error("hud.set_scale: Bad scale value: %1.3f\n", scale);

                HUD_SetScale(scale);
            })
        .addFunction(
            "set_alpha", +[](double alpha) { HUD_SetAlpha(alpha); })
        .addFunction(
            "set_render_who",
            +[](int index) {
                if (index < 0 || index >= numplayers)
                    I_Error("hud.set_render_who: bad index value: %d (numplayers=%d)\n",
                            index,
                            numplayers);

                if (index == 0)
                {
                    ui_hud_who = players[consoleplayer];
                    return;
                }

                int who = displayplayer;

                for (; index > 1; index--)
                {
                    do
                    {
                        who = (who + 1) % MAXPLAYERS;
                    } while (players[who] == NULL);
                }

                ui_hud_who = players[who];
            })
        .addFunction(
            "automap_color",
            +[](int which, luabridge::LuaRef v) {
                if (which < 1 || which > AM_NUM_COLORS)
                    I_Error("hud.automap_color: bad color number: %d\n", which);

                which--;

                rgbcol_t rgb = Vec3ToColor(v);

                AM_SetColor(which, rgb);
            })
        .addFunction(
            "automap_option",
            +[](int which, int value) {
                if (which < 1 || which > 7)
                    I_Error("hud.automap_color: bad color number: %d\n", which);

                which--;

                if (value <= 0)
                    ui_hud_automap_flags[0] |= (1 << which);
                else
                    ui_hud_automap_flags[1] |= (1 << which);
            })
        .addFunction(
            "automap_zoom", +[](float zoom) { ui_hud_automap_zoom = CLAMP(0.2f, zoom, 100.0f); })
        .addFunction(
            "automap_player_arrow", +[](int arrow) { AM_SetArrow((automap_arrow_e) arrow); })

        // drawing functions
        .addFunction(
            "solid_box",
            +[](float x, float y, float w, float h, luabridge::LuaRef color) {
                HUD_SolidBox(x, y, x + w, y + h, Vec3ToColor(color));
            })
        .addFunction(
            "solid_line",
            +[](float x1, float y1, float x2, float y2, luabridge::LuaRef color) {
                HUD_SolidLine(x1, y1, x2, y2, Vec3ToColor(color));
            })
        .addFunction(
            "thin_box",
            +[](float x, float y, float w, float h, luabridge::LuaRef color) {
                HUD_ThinBox(x, y, x + w, y + h, Vec3ToColor(color));
            })
        .addFunction(
            "gradient_box",
            +[](float x,
                float y,
                float w,
                float h,
                luabridge::LuaRef tl,
                luabridge::LuaRef bl,
                luabridge::LuaRef tr,
                luabridge::LuaRef br) {
                rgbcol_t cols[4];

                cols[0] = Vec3ToColor(tl);
                cols[1] = Vec3ToColor(bl);
                cols[2] = Vec3ToColor(tr);
                cols[3] = Vec3ToColor(br);

                HUD_GradientBox(x, y, x + w, y + h, cols);
            })
        .addFunction(
            "draw_image",
            +[](float x, float y, const char* name, int noOffset = 0) {
                const image_c* img = W_ImageLookup(name, INS_Graphic);

                if (img)
                {
                    if (noOffset)
                        HUD_DrawImageNoOffset(x, y, img);
                    else
                        HUD_DrawImage(x, y, img);
                }
            })
        .addFunction(
            "stretch_image",
            +[](float x, float y, float w, float h, const char* name, int noOffset = 0) {
                const image_c* img = W_ImageLookup(name, INS_Graphic);

                if (img)
                {
                    if (noOffset)
                        HUD_StretchImageNoOffset(x, y, w, h, img, 0.0, 0.0);
                    else
                        HUD_StretchImage(x, y, w, h, img, 0.0, 0.0);
                }
            })
        .addFunction(
            "scroll_image",
            +[](float x, float y, const char* name, float sx, float sy, int noOffset = 0) {
                const image_c* img = W_ImageLookup(name, INS_Graphic);

                if (img)
                {
                    if (noOffset)
                        HUD_ScrollImageNoOffset(
                            x, y, img, -sx, -sy); // Invert sx/sy so that user can enter positive X
                                                  // for right and positive Y for up
                    else
                        HUD_ScrollImage(
                            x, y, img, -sx, -sy); // Invert sx/sy so that user can enter positive X
                                                  // for right and positive Y for up
                }
            })
        .addFunction(
            "tile_image",
            +[](float x,
                float y,
                float w,
                float h,
                const char* name,
                float offset_x,
                float offset_y) {
                const image_c* img = W_ImageLookup(name, INS_Texture);

                if (img)
                {
                    HUD_TileImage(x, y, w, h, img, offset_x, offset_y);
                }
            })
        .addFunction(
            "draw_text",
            +[](float x, float y, const char* str, float size = 0.0f) {
                HUD_DrawText(x, y, str, size);
            })
        .addFunction(
            "draw_num2",
            +[](float x, float y, int len, int num, float size = 0.0f) {
                if (len < 1 || len > 20)
                    I_Error("hud.draw_number: bad field length: %d\n", len);

                bool is_neg = false;

                if (num < 0 && len > 1)
                {
                    is_neg = true;
                    len--;
                }

                // build the integer backwards
                char buffer[200];
                char* pos = &buffer[sizeof(buffer) - 4];

                *--pos = 0;

                if (num == 0)
                {
                    *--pos = '0';
                }
                else
                {
                    for (; num > 0 && len > 0; num /= 10, len--)
                        *--pos = '0' + (num % 10);

                    if (is_neg)
                        *--pos = '-';
                }

                HUD_SetAlignment(+1, -1);
                HUD_DrawText(x, y, pos, size);
                HUD_SetAlignment();
            })
        .addFunction(
            "draw_number",
            +[](float x, float y, int len, int num, int align_right, float size = 0.0f) {
                if (len < 1 || len > 20)
                    I_Error("hud.draw_number: bad field length: %d\n", len);

                bool is_neg = false;

                if (num < 0 && len > 1)
                {
                    is_neg = true;
                    len--;
                }

                // build the integer backwards

                char buffer[200];
                char* pos = &buffer[sizeof(buffer) - 4];

                *--pos = 0;

                if (num == 0)
                {
                    *--pos = '0';
                }
                else
                {
                    for (; num > 0 && len > 0; num /= 10, len--)
                        *--pos = '0' + (num % 10);

                    if (is_neg)
                        *--pos = '-';
                }

                if (align_right == 0)
                {
                    HUD_DrawText(x, y, pos, size);
                }
                else
                {
                    HUD_SetAlignment(+1, -1);
                    HUD_DrawText(x, y, pos, size);
                    HUD_SetAlignment();
                }
            })
        .addFunction(
            "game_paused",
            +[]() {
                if (paused || menuactive || rts_menuactive || time_stop_active || erraticism_active)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            })
        .addFunction(
            "erraticism_active", +[]() { return erraticism_active ? true : false; })
        .addFunction(
            "time_stop_active", +[]() { return time_stop_active ? true : false; })
        .addFunction(
            "screen_aspect", +[]() { return std::ceil(v_pixelaspect.f * 100.0) / 100.0; })
        .addFunction(
            "render_world",
            +[](float x, float y, float w, float h, int flags = 0) {
                HUD_RenderWorld(x, y, w, h, ui_hud_who->mo, flags);
            })
        .addFunction(
            "render_automap",
            +[](float x, float y, float w, float h, int flags = 0) {
                int old_state;
                float old_zoom;

                AM_GetState(&old_state, &old_zoom);

                int new_state = old_state;
                new_state &= ~ui_hud_automap_flags[0];
                new_state |= ui_hud_automap_flags[1];

                float new_zoom = old_zoom;
                if (ui_hud_automap_zoom > 0.1)
                    new_zoom = ui_hud_automap_zoom;

                AM_SetState(new_state, new_zoom);

                HUD_RenderAutomap(x, y, w, h, ui_hud_who->mo, flags);

                AM_SetState(old_state, old_zoom);
            })
        .addFunction(
            "play_sound",
            +[](const char* name) {
                sfx_t* fx = sfxdefs.GetEffect(name);

                if (fx)
                    S_StartFX(fx);
                else
                    I_Warning("hud.play_sound: unknown sfx '%s'\n", name);
            })
        .addFunction(
            "get_average_color",
            +[](const char* name,
                double from_x = -1,
                double to_x = 1000000,
                double from_y = -1,
                double to_y = 1000000) {
                const byte* what_palette = (const byte*) &playpal_data[0];
                const image_c* tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
                if (tmp_img_c->source_palette >= 0)
                    what_palette = (const byte*) W_LoadLump(tmp_img_c->source_palette);
                epi::image_data_c* tmp_img_data = R_PalettisedToRGB(
                    ReadAsEpiBlock((image_c*) tmp_img_c), what_palette, tmp_img_c->opacity);
                u8_t* temp_rgb = new u8_t[3];
                tmp_img_data->AverageColor(temp_rgb, from_x, to_x, from_y, to_y);

                // note we should be using vec3 here
                luabridge::LuaRef rgb = luabridge::newTable(hud_state);
                rgb[1] = temp_rgb[0];
                rgb[2] = temp_rgb[1];
                rgb[3] = temp_rgb[2];

                delete tmp_img_data;
                delete[] temp_rgb;

                return rgb;
            })
        .addFunction(
            "get_average_top_border_color",
            +[](const char* name) {
                const byte* what_palette = (const byte*) &playpal_data[0];
                const image_c* tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
                if (tmp_img_c->source_palette >= 0)
                    what_palette = (const byte*) W_LoadLump(tmp_img_c->source_palette);
                epi::image_data_c* tmp_img_data = R_PalettisedToRGB(
                    ReadAsEpiBlock((image_c*) tmp_img_c), what_palette, tmp_img_c->opacity);
                u8_t* temp_rgb = new u8_t[3];
                tmp_img_data->AverageColor(
                    temp_rgb, 0, tmp_img_c->actual_w, tmp_img_c->actual_h - 1, tmp_img_c->actual_h);
                // note we should be using vec3 here
                luabridge::LuaRef rgb = luabridge::newTable(hud_state);
                rgb[1] = temp_rgb[0];
                rgb[2] = temp_rgb[1];
                rgb[3] = temp_rgb[2];
                delete tmp_img_data;
                delete[] temp_rgb;
                return rgb;
            })
        .addFunction(
            "get_average_bottom_border_color",
            +[](const char* name) {
                const byte* what_palette = (const byte*) &playpal_data[0];
                const image_c* tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
                if (tmp_img_c->source_palette >= 0)
                    what_palette = (const byte*) W_LoadLump(tmp_img_c->source_palette);
                epi::image_data_c* tmp_img_data = R_PalettisedToRGB(
                    ReadAsEpiBlock((image_c*) tmp_img_c), what_palette, tmp_img_c->opacity);
                u8_t* temp_rgb = new u8_t[3];
                tmp_img_data->AverageColor(temp_rgb, 0, tmp_img_c->actual_w, 0, 1);
                // note we should be using vec3 here
                luabridge::LuaRef rgb = luabridge::newTable(hud_state);
                rgb[1] = temp_rgb[0];
                rgb[2] = temp_rgb[1];
                rgb[3] = temp_rgb[2];
                delete tmp_img_data;
                delete[] temp_rgb;
                return rgb;
            })
        .addFunction(
            "get_lightest_color",
            +[](const char* name,
                double from_x = -1,
                double to_x = 1000000,
                double from_y = -1,
                double to_y = 1000000) {
                const byte* what_palette = (const byte*) &playpal_data[0];
                const image_c* tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
                if (tmp_img_c->source_palette >= 0)
                    what_palette = (const byte*) W_LoadLump(tmp_img_c->source_palette);
                epi::image_data_c* tmp_img_data = R_PalettisedToRGB(
                    ReadAsEpiBlock((image_c*) tmp_img_c), what_palette, tmp_img_c->opacity);
                u8_t* temp_rgb = new u8_t[3];
                tmp_img_data->LightestColor(temp_rgb, from_x, to_x, from_y, to_y);
                // note we should be using vec3 here
                luabridge::LuaRef rgb = luabridge::newTable(hud_state);
                rgb[1] = temp_rgb[0];
                rgb[2] = temp_rgb[1];
                rgb[3] = temp_rgb[2];
                delete tmp_img_data;
                delete[] temp_rgb;
                return rgb;
            })
        .addFunction(
            "get_darkest_color",
            +[](const char* name,
                double from_x = -1,
                double to_x = 1000000,
                double from_y = -1,
                double to_y = 1000000) {
                const byte* what_palette = (const byte*) &playpal_data[0];
                const image_c* tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
                if (tmp_img_c->source_palette >= 0)
                    what_palette = (const byte*) W_LoadLump(tmp_img_c->source_palette);
                epi::image_data_c* tmp_img_data = R_PalettisedToRGB(
                    ReadAsEpiBlock((image_c*) tmp_img_c), what_palette, tmp_img_c->opacity);
                u8_t* temp_rgb = new u8_t[3];
                tmp_img_data->DarkestColor(temp_rgb, from_x, to_x, from_y, to_y);
                // note we should be using vec3 here
                luabridge::LuaRef rgb = luabridge::newTable(hud_state);
                rgb[1] = temp_rgb[0];
                rgb[2] = temp_rgb[1];
                rgb[3] = temp_rgb[2];
                delete tmp_img_data;
                delete[] temp_rgb;
                return rgb;
            })
        .addFunction(
            "get_average_hue",
            +[](const char* name,
                double from_x = -1,
                double to_x = 1000000,
                double from_y = -1,
                double to_y = 1000000) {
                const byte* what_palette = (const byte*) &playpal_data[0];
                const image_c* tmp_img_c = W_ImageLookup(name, INS_Graphic, 0);
                if (tmp_img_c->source_palette >= 0)
                    what_palette = (const byte*) W_LoadLump(tmp_img_c->source_palette);
                epi::image_data_c* tmp_img_data = R_PalettisedToRGB(
                    ReadAsEpiBlock((image_c*) tmp_img_c), what_palette, tmp_img_c->opacity);
                u8_t* temp_rgb = new u8_t[3];
                tmp_img_data->AverageHue(temp_rgb, NULL, from_x, to_x, from_y, to_y);
                luabridge::LuaRef rgb = luabridge::newTable(hud_state);
                rgb[1] = temp_rgb[0];
                rgb[2] = temp_rgb[1];
                rgb[3] = temp_rgb[2];
                delete tmp_img_data;
                delete[] temp_rgb;
                return rgb;
            })
        .addFunction(
            "rts_enable",
            +[](const std::string tag) {
                if (!tag.empty())
                    RAD_EnableByTag(NULL, tag.c_str(), false);
            })
        .addFunction(
            "rts_isactive",
            +[](const std::string tag) {
                if (!tag.empty())
                {
                    if (RAD_IsActiveByTag(NULL, tag.c_str()))
                        return true;
                    else
                        return false;
                }

                return false;
            })
        .addFunction(
            "get_image_width",
            +[](const char* name) {
                const image_c* img = W_ImageLookup(name, INS_Graphic);

                if (img)
                {
                    return (int) HUD_GetImageWidth(img);
                }

                return 0;
            })
        .addFunction(
            "get_image_height",
            +[](const char* name) {
                const image_c* img = W_ImageLookup(name, INS_Graphic);

                if (img)
                {
                    return (int) HUD_GetImageHeight(img);
                }

                return 0;
            })

        .endNamespace()
        .endNamespace();
}

void LUA_Coal_NewGame(void)
{
    SYS_ASSERT(hud_state);
    luabridge::getGlobal(hud_state, "new_game")();
}

void LUA_Coal_LoadGame(void)
{
    SYS_ASSERT(hud_state);

    // Need to set these to prevent NULL references if using any player.xxx in the load_level hook
    ui_hud_who = players[displayplayer];
    lua_ui_player_who = players[displayplayer];

    luabridge::getGlobal(hud_state, "load_game")();
}

void LUA_Coal_SaveGame(void)
{
    SYS_ASSERT(hud_state);

    luabridge::getGlobal(hud_state, "save_game")();
}

void LUA_Coal_BeginLevel(void)
{
    SYS_ASSERT(hud_state);

    // Need to set these to prevent NULL references if using player.xxx in the begin_level hook
    ui_hud_who = players[displayplayer];
    lua_ui_player_who = players[displayplayer];
    luabridge::getGlobal(hud_state, "begin_level")();
}

void LUA_Coal_EndLevel(void)
{
    SYS_ASSERT(hud_state);
    luabridge::getGlobal(hud_state, "end_level")();
}

void LUA_Coal_RunHud(void)
{
    SYS_ASSERT(hud_state);

    HUD_Reset();

    ui_hud_who = players[displayplayer];
    lua_ui_player_who = players[displayplayer];

    ui_hud_automap_flags[0] = 0;
    ui_hud_automap_flags[1] = 0;
    ui_hud_automap_zoom = -1;

    luabridge::getGlobal(hud_state, "draw_all")();

    HUD_Reset();
}
