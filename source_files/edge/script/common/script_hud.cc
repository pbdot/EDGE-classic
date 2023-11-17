//------------------------------------------------------------------------
//  COAL HUD module
//------------------------------------------------------------------------
//
//  Copyright (c) 2006-2023  The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

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

#include <cmath>
#include "script_hud.h"

extern cvar_c r_doubleframes;

// Needed for color functions
extern epi::image_data_c *ReadAsEpiBlock(image_c *rim);
extern epi::image_data_c *R_PalettisedToRGB(epi::image_data_c *src, const byte *palette, int opacity);


extern std::string w_map_title;

extern bool erraticism_active;

int       ui_hud_automap_flags[2]; // 0 = disabled, 1 = enabled
float     ui_hud_automap_zoom;
player_t *ui_hud_who = NULL;


namespace script_common
{

//------------------------------------------------------------------------

static rgbcol_t SC_VectorToColor(const epi::vec3_c &v)
{
    if (v.x < 0)
        return RGB_NO_VALUE;

    int r = CLAMP(0, (int)v.x, 255);
    int g = CLAMP(0, (int)v.y, 255);
    int b = CLAMP(0, (int)v.z, 255);

    rgbcol_t rgb = RGB_MAKE(r, g, b);

    // ensure we don't get the "no color" value by mistake
    if (rgb == RGB_NO_VALUE)
        rgb ^= 0x000101;

    return rgb;
}

//------------------------------------------------------------------------
//  HUD MODULE
//------------------------------------------------------------------------

// hud.coord_sys(w, h)
//

/* FUNKY
void SC_coord_sys(coal::vm_c *vm, int argc)
{
    (void)argc;

    int w = (int)*vm->AccessParam(0);
    int h = (int)*vm->AccessParam(1);

    if (w < 64 || h < 64)
        I_Error("Bad hud.coord_sys size: %dx%d\n", w, h);

    HUD_SetCoordSys(w, h);

    VM_SetFloat(ui_vm, "hud", "x_left", hud_x_left);
    VM_SetFloat(ui_vm, "hud", "x_right", hud_x_right);
}
*/

// hud.game_mode()
//
const char *SC_game_mode()
{
    if (DEATHMATCH())
        return "dm";
    else if (COOP_MATCH())
        return "coop";
    else
        return "sp";
}

// hud.game_name()
//
const char *SC_game_name()
{
    gamedef_c *g = currmap->episode;
    SYS_ASSERT(g);

    return g->name.c_str();
}

// hud.map_name()
//
const char *SC_map_name()
{
    return currmap->name.c_str();
}

// hud.map_title()
//
const char *SC_map_title()
{
    return w_map_title.c_str();
}

// hud.map_author()
//
const char *SC_map_author()
{
    return currmap->author.c_str();
}

// hud.which_hud()
//
int SC_which_hud()
{
    return screen_hud;
}

// hud.check_automap()
//
bool SC_check_automap()
{
    return automapactive;
}

// hud.get_time()
//
double SC_get_time()
{
    int time = I_GetTime() / (r_doubleframes.d ? 2 : 1);
    return (double)time;
}

// hud.text_font(name)
//
void SC_text_font(const char *font_name)
{
    fontdef_c *DEF = fontdefs.Lookup(font_name);
    SYS_ASSERT(DEF);

    if (!DEF)
        I_Error("hud.text_font: Bad font name: %s\n", font_name);

    font_c *font = hu_fonts.Lookup(DEF);
    SYS_ASSERT(font);

    if (!font)
        I_Error("hud.text_font: Bad font name: %s\n", font_name);

    HUD_SetFont(font);
}

// hud.text_color(rgb)
//
void SC_text_color(const epi::vec3_c &v)
{
    HUD_SetTextColor(SC_VectorToColor(v));
}

// hud.set_scale(value)
//
void SC_set_scale(float scale)
{
    if (scale <= 0)
        I_Error("hud.set_scale: Bad scale value: %1.3f\n", scale);

    HUD_SetScale(scale);
}

// hud.set_alpha(value)
//
void SC_set_alpha(float alpha)
{
    HUD_SetAlpha(alpha);
}

// hud.solid_box(x, y, w, h, color)
//
void SC_solid_box(float x, float y, float w, float h, const epi::vec3_c &rgb)
{
    rgbcol_t color = SC_VectorToColor(rgb);
    HUD_SolidBox(x, y, x + w, y + h, color);
}

// hud.solid_line(x1, y1, x2, y2, color)
//
void SC_solid_line(float x1, float y1, float x2, float y2, const epi::vec3_c &rgb)
{
    rgbcol_t color = SC_VectorToColor(rgb);

    HUD_SolidLine(x1, y1, x2, y2, color);
}

// hud.thin_box(x, y, w, h, color)
//
void SC_thin_box(float x, float y, float w, float h, const epi::vec3_c &rgb)
{
    rgbcol_t color = SC_VectorToColor(rgb);
    HUD_ThinBox(x, y, x + w, y + h, color);
}

// hud.gradient_box(x, y, w, h, TL, BL, TR, BR)
//
void SC_gradient_box(float x, float y, float w, float h, const epi::vec3_c &tl, const epi::vec3_c &bl,
                     const epi::vec3_c &tr, const epi::vec3_c &br)
{
    rgbcol_t cols[4];

    cols[0] = SC_VectorToColor(tl);
    cols[1] = SC_VectorToColor(bl);
    cols[2] = SC_VectorToColor(tr);
    cols[3] = SC_VectorToColor(br);

    HUD_GradientBox(x, y, x + w, y + h, cols);
}

// hud.draw_image(x, y, name, [noOffset])
// if we specify noOffset then it ignores
// X and Y offsets from doom or images.ddf
//
void SC_draw_image(float x, float y, const char *name, bool noOffset)
{
    const image_c *img = W_ImageLookup(name, INS_Graphic);

    if (img)
    {
        if (noOffset)
            HUD_DrawImageNoOffset(x, y, img);
        else
            HUD_DrawImage(x, y, img);
    }
}

// Dasho 2022: Same as above but adds x/y texcoord scrolling
// hud.scroll_image(x, y, name, sx, sy, [noOffset])
//
void SC_scroll_image(float x, float y, const char *name, float sx, float sy, bool noOffset)
{
    const image_c *img = W_ImageLookup(name, INS_Graphic);

    if (img)
    {
        if (noOffset)
            HUD_ScrollImageNoOffset(
                x, y, img, -sx, -sy); // Invert sx/sy so that user can enter positive X for right and positive Y for up
        else
            HUD_ScrollImage(x, y, img, -sx,
                            -sy); // Invert sx/sy so that user can enter positive X for right and positive Y for up
    }
}

// hud.stretch_image(x, y, w, h, name, [noOffset])
// if we specify noOffset then it ignores
// X and Y offsets from doom or images.ddf
//
void SC_stretch_image(float x, float y, float w, float h, const char *name, bool noOffset)
{
    const image_c *img = W_ImageLookup(name, INS_Graphic);

    if (img)
    {
        if (noOffset)
            HUD_StretchImageNoOffset(x, y, w, h, img, 0.0, 0.0);
        else
            HUD_StretchImage(x, y, w, h, img, 0.0, 0.0);
    }
}

// hud.tile_image(x, y, w, h, name, offset_x, offset_y)
//
void SC_tile_image(float x, float y, float w, float h, const char *name, float offset_x, float offset_y)
{

    const image_c *img = W_ImageLookup(name, INS_Texture);

    if (img)
    {
        HUD_TileImage(x, y, w, h, img, offset_x, offset_y);
    }
}

// hud.draw_text(x, y, str, [size])
//
void SC_draw_text(float x, float y, const char *str, float size)
{
    HUD_DrawText(x, y, str, size);
}

// hud.draw_num2(x, y, len, num, [size])
//
void SC_draw_num2(float x, float y, int len, int num, int size)
{
    if (len < 1 || len > 20)
        I_Error("hud.draw_number: bad field length: %d\n", len);

    bool is_neg = false;

    if (num < 0 && len > 1)
    {
        is_neg = true;
        len--;
    }

    // build the integer backwards

    char  buffer[200];
    char *pos = &buffer[sizeof(buffer) - 4];

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
}

// Lobo November 2021:
// hud.draw_number(x, y, len, num, align_right, [size])
//
void SC_draw_number(float x, float y, int len, int num, int align_right, float size)
{

    if (len < 1 || len > 20)
        I_Error("hud.draw_number: bad field length: %d\n", len);

    bool is_neg = false;

    if (num < 0 && len > 1)
    {
        is_neg = true;
        len--;
    }

    // build the integer backwards

    char  buffer[200];
    char *pos = &buffer[sizeof(buffer) - 4];

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
}

// hud.game_paused()
//
bool SC_game_paused()
{
    if (paused || menuactive || rts_menuactive || time_stop_active || erraticism_active)
    {
        true;
    }

    return false;
}

// hud.erraticism_active()
//
bool SC_erraticism_active()
{
    return erraticism_active;
}

// hud.time_stop_active()
//
bool SC_time_stop_active()
{
    return time_stop_active;
}

// hud.render_world(x, y, w, h, [flags])
//
void SC_render_world(float x, float y, float w, float h, int flags)
{

    HUD_RenderWorld(x, y, w, h, ui_hud_who->mo, flags);
}

// hud.render_automap(x, y, w, h, [flags])
//
void SC_render_automap(float x, float y, float w, float h, int flags)
{

    int   old_state;
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
}

// hud.automap_color(which, color)
//
void SC_automap_color(int which, const epi::vec3_c &rgb)
{

    if (which < 1 || which > AM_NUM_COLORS)
        I_Error("hud.automap_color: bad color number: %d\n", which);

    which--;

    rgbcol_t color = SC_VectorToColor(rgb);

    AM_SetColor(which, color);
}

// hud.automap_option(which, value)
//
void SC_automap_option(int which, int value)
{
    if (which < 1 || which > 7)
        I_Error("hud.automap_color: bad color number: %d\n", which);

    which--;

    if (value <= 0)
        ui_hud_automap_flags[0] |= (1 << which);
    else
        ui_hud_automap_flags[1] |= (1 << which);
}

// hud.automap_zoom(value)
//
void SC_automap_zoom(float zoom)
{
    // impose a very broad limit
    ui_hud_automap_zoom = CLAMP(0.2f, zoom, 100.0f);
}

// hud.automap_player_arrow(type)
//
void SC_automap_player_arrow(int arrow)
{
    AM_SetArrow((automap_arrow_e)arrow);
}

// hud.set_render_who(index)
//
void SC_set_render_who(int index)
{
    if (index < 0 || index >= numplayers)
        I_Error("hud.set_render_who: bad index value: %d (numplayers=%d)\n", index, numplayers);

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
}

// hud.play_sound(name)
//
void SC_play_sound(const char *name)
{
    sfx_t *fx = sfxdefs.GetEffect(name);

    if (fx)
        S_StartFX(fx);
    else
        I_Warning("hud.play_sound: unknown sfx '%s'\n", name);
}

// hud.screen_aspect()
//
float SC_screen_aspect()
{
    return std::ceil(v_pixelaspect.f * 100.0) / 100.0;
}

epi::vec3_c SC_get_average_color(const char *name, float from_x, float to_x, float from_y,
                                 float to_y)
{
    double rgb[3];

    const byte    *what_palette = (const byte *)&playpal_data[0];
    const image_c *tmp_img_c    = W_ImageLookup(name, INS_Graphic, 0);
    if (tmp_img_c->source_palette >= 0)
        what_palette = (const byte *)W_LoadLump(tmp_img_c->source_palette);
    epi::image_data_c *tmp_img_data =
        R_PalettisedToRGB(ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
    u8_t *temp_rgb = new u8_t[3];
    tmp_img_data->AverageColor(temp_rgb, from_x, to_x, from_y, to_y);
    rgb[0] = temp_rgb[0];
    rgb[1] = temp_rgb[1];
    rgb[2] = temp_rgb[2];
    delete tmp_img_data;
    delete[] temp_rgb;
    return epi::vec3_c(rgb[0], rgb[1], rgb[2]);
}

epi::vec3_c SC_get_lightest_color(const char *name, float from_x, float to_x, float from_y,
                                  float to_y)
{
    double         rgb[3];
    const byte    *what_palette = (const byte *)&playpal_data[0];
    const image_c *tmp_img_c    = W_ImageLookup(name, INS_Graphic, 0);
    if (tmp_img_c->source_palette >= 0)
        what_palette = (const byte *)W_LoadLump(tmp_img_c->source_palette);
    epi::image_data_c *tmp_img_data =
        R_PalettisedToRGB(ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
    u8_t *temp_rgb = new u8_t[3];
    tmp_img_data->LightestColor(temp_rgb, from_x, to_x, from_y, to_y);
    rgb[0] = temp_rgb[0];
    rgb[1] = temp_rgb[1];
    rgb[2] = temp_rgb[2];
    delete tmp_img_data;
    delete[] temp_rgb;
    return epi::vec3_c(rgb[0], rgb[1], rgb[2]);
}

epi::vec3_c SC_get_darkest_color(const char *name, float from_x, float to_x, float from_y,
                                 float to_y)
{
    double         rgb[3];
    const byte    *what_palette = (const byte *)&playpal_data[0];
    const image_c *tmp_img_c    = W_ImageLookup(name, INS_Graphic, 0);
    if (tmp_img_c->source_palette >= 0)
        what_palette = (const byte *)W_LoadLump(tmp_img_c->source_palette);
    epi::image_data_c *tmp_img_data =
        R_PalettisedToRGB(ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
    u8_t *temp_rgb = new u8_t[3];
    tmp_img_data->DarkestColor(temp_rgb, from_x, to_x, from_y, to_y);
    rgb[0] = temp_rgb[0];
    rgb[1] = temp_rgb[1];
    rgb[2] = temp_rgb[2];
    delete tmp_img_data;
    delete[] temp_rgb;
    return epi::vec3_c(rgb[0], rgb[1], rgb[2]);
}

epi::vec3_c SC_get_average_hue(const char *name, float from_x, float to_x, float from_y,
                               float to_y)
{
    double         rgb[3];
    const byte    *what_palette = (const byte *)&playpal_data[0];
    const image_c *tmp_img_c    = W_ImageLookup(name, INS_Graphic, 0);
    if (tmp_img_c->source_palette >= 0)
        what_palette = (const byte *)W_LoadLump(tmp_img_c->source_palette);
    epi::image_data_c *tmp_img_data =
        R_PalettisedToRGB(ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
    u8_t *temp_rgb = new u8_t[3];
    tmp_img_data->AverageHue(temp_rgb, NULL, from_x, to_x, from_y, to_y);
    rgb[0] = temp_rgb[0];
    rgb[1] = temp_rgb[1];
    rgb[2] = temp_rgb[2];
    delete tmp_img_data;
    delete[] temp_rgb;
    return epi::vec3_c(rgb[0], rgb[1], rgb[2]);
}

// These two aren't really needed anymore with the AverageColor rework, but keeping them in case COALHUDS in the wild
// use them - Dasho
epi::vec3_c SC_get_average_top_border_color(const char *name)
{

    double rgb[3];

    const byte    *what_palette = (const byte *)&playpal_data[0];
    const image_c *tmp_img_c    = W_ImageLookup(name, INS_Graphic, 0);
    if (tmp_img_c->source_palette >= 0)
        what_palette = (const byte *)W_LoadLump(tmp_img_c->source_palette);
    epi::image_data_c *tmp_img_data =
        R_PalettisedToRGB(ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
    u8_t *temp_rgb = new u8_t[3];
    tmp_img_data->AverageColor(temp_rgb, 0, tmp_img_c->actual_w, tmp_img_c->actual_h - 1, tmp_img_c->actual_h);
    rgb[0] = temp_rgb[0];
    rgb[1] = temp_rgb[1];
    rgb[2] = temp_rgb[2];
    delete tmp_img_data;
    delete[] temp_rgb;
    return epi::vec3_c(rgb[0], rgb[1], rgb[2]);
}
epi::vec3_c SC_get_average_bottom_border_color(const char *name)
{
    double         rgb[3];
    const byte    *what_palette = (const byte *)&playpal_data[0];
    const image_c *tmp_img_c    = W_ImageLookup(name, INS_Graphic, 0);
    if (tmp_img_c->source_palette >= 0)
        what_palette = (const byte *)W_LoadLump(tmp_img_c->source_palette);
    epi::image_data_c *tmp_img_data =
        R_PalettisedToRGB(ReadAsEpiBlock((image_c *)tmp_img_c), what_palette, tmp_img_c->opacity);
    u8_t *temp_rgb = new u8_t[3];
    tmp_img_data->AverageColor(temp_rgb, 0, tmp_img_c->actual_w, 0, 1);
    rgb[0] = temp_rgb[0];
    rgb[1] = temp_rgb[1];
    rgb[2] = temp_rgb[2];
    delete tmp_img_data;
    delete[] temp_rgb;
    return epi::vec3_c(rgb[0], rgb[1], rgb[2]);
}

// hud.rts_enable(tag)
//
void SC_rts_enable(const char *name)
{
    if (name && strlen(name))
        RAD_EnableByTag(NULL, name, false);
}

// hud.rts_disable(tag)
//
void SC_rts_disable(const char *name)
{

    if (name && strlen(name))
        RAD_EnableByTag(NULL, name, true);
}

// hud.rts_isactive(tag)
//
bool SC_rts_isactive(const char *name)
{
    if (!name || !strlen(name))
    {
        return false;
    }

    return RAD_IsActiveByTag(NULL, name);
}

// hud.get_image_width(name)
//
int SC_get_image_width(const char *name)
{
    if (!name || !strlen(name))
    {
        return 0;
    }
    const image_c *img = W_ImageLookup(name, INS_Graphic);

    if (img)
    {
        return HUD_GetImageWidth(img);
    }

    return 0;
}

// hud.get_image_height(name)
//
int SC_get_image_height(const char *name)
{
    if (!name || !strlen(name))
    {
        return 0;
    }

    const image_c *img = W_ImageLookup(name, INS_Graphic);

    if (img)
    {
        return HUD_GetImageHeight(img);
    }

    return 0;
}

} // namespace script_common
