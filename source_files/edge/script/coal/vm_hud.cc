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

#include "coal.h"

#include "main.h"
#include "font.h"

#include "image_data.h"

#include "vm_coal.h"
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

#include "../common/script_hud.h"
using namespace script_common;

extern coal::vm_c *ui_vm;

extern void VM_SetFloat(coal::vm_c *vm, const char *mod, const char *name, double value);
extern void VM_CallFunction(coal::vm_c *vm, const char *name);

//------------------------------------------------------------------------
//  HUD MODULE
//------------------------------------------------------------------------

inline epi::vec3_c VM_VectorToVec3(double *v)
{
    SYS_ASSERT(v);
    return epi::vec3_c(v[0], v[1], v[2]);
}

// hud.coord_sys(w, h)
//
static void HD_coord_sys(coal::vm_c *vm, int argc)
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

// hud.game_mode()
//
static void HD_game_mode(coal::vm_c *vm, int argc)
{
    (void)argc;
    vm->ReturnString(SC_game_mode());
}

// hud.game_name()
//
static void HD_game_name(coal::vm_c *vm, int argc)
{
    (void)argc;
    vm->ReturnString(SC_game_name());
}

// hud.map_name()
//
static void HD_map_name(coal::vm_c *vm, int argc)
{
    (void)argc;
    vm->ReturnString(SC_map_name());
}

// hud.map_title()
//
static void HD_map_title(coal::vm_c *vm, int argc)
{
    (void)argc;
    vm->ReturnString(SC_map_title());
}

// hud.map_author()
//
static void HD_map_author(coal::vm_c *vm, int argc)
{
    (void)argc;
    vm->ReturnString(SC_map_author());
}

// hud.which_hud()
//
static void HD_which_hud(coal::vm_c *vm, int argc)
{
    (void)argc;

    vm->ReturnFloat(SC_which_hud());
}

// hud.check_automap()
//
static void HD_check_automap(coal::vm_c *vm, int argc)
{
    (void)argc;

    vm->ReturnFloat(SC_check_automap() ? 1 : 0);
}

// hud.get_time()
//
static void HD_get_time(coal::vm_c *vm, int argc)
{
    (void)argc;

    vm->ReturnFloat(SC_get_time());
}

// hud.text_font(name)
//
static void HD_text_font(coal::vm_c *vm, int argc)
{
    (void)argc;

    const char *font_name = vm->AccessParamString(0);
    SC_text_font(font_name);
}

// hud.text_color(rgb)
//
static void HD_text_color(coal::vm_c *vm, int argc)
{
    (void)argc;

    double *v = vm->AccessParam(0);
    if (v)
    {
        SC_text_color(epi::vec3_c(v[0], v[1], v[2]));
    }
}

// hud.set_scale(value)
//
static void HD_set_scale(coal::vm_c *vm, int argc)
{
    (void)argc;
    SC_set_scale(*vm->AccessParam(0));
}

// hud.set_alpha(value)
//
static void HD_set_alpha(coal::vm_c *vm, int argc)
{
    (void)argc;

    float alpha = *vm->AccessParam(0);

    HUD_SetAlpha(alpha);
}

// hud.solid_box(x, y, w, h, color)
//
static void HD_solid_box(coal::vm_c *vm, int argc)
{
    (void)argc;

    float       x   = *vm->AccessParam(0);
    float       y   = *vm->AccessParam(1);
    float       w   = *vm->AccessParam(2);
    float       h   = *vm->AccessParam(3);
    epi::vec3_c rgb = VM_VectorToVec3(vm->AccessParam(4));

    SC_solid_box(x, y, w, h, rgb);
}

// hud.solid_line(x1, y1, x2, y2, color)
//
static void HD_solid_line(coal::vm_c *vm, int argc)
{
    float       x1  = *vm->AccessParam(0);
    float       y1  = *vm->AccessParam(1);
    float       x2  = *vm->AccessParam(2);
    float       y2  = *vm->AccessParam(3);
    epi::vec3_c rgb = VM_VectorToVec3(vm->AccessParam(4));

    SC_solid_line(x1, y1, x2, y2, rgb);
}

// hud.thin_box(x, y, w, h, color)
//
static void HD_thin_box(coal::vm_c *vm, int argc)
{
    (void)argc;

    float       x   = *vm->AccessParam(0);
    float       y   = *vm->AccessParam(1);
    float       w   = *vm->AccessParam(2);
    float       h   = *vm->AccessParam(3);
    epi::vec3_c rgb = VM_VectorToVec3(vm->AccessParam(4));

    SC_thin_box(x, y, w, h, rgb);
}

// hud.gradient_box(x, y, w, h, TL, BL, TR, BR)
//
static void HD_gradient_box(coal::vm_c *vm, int argc)
{
    (void)argc;

    float x = *vm->AccessParam(0);
    float y = *vm->AccessParam(1);
    float w = *vm->AccessParam(2);
    float h = *vm->AccessParam(3);

    epi::vec3_c tl = VM_VectorToVec3(vm->AccessParam(4));
    epi::vec3_c bl = VM_VectorToVec3(vm->AccessParam(5));
    epi::vec3_c tr = VM_VectorToVec3(vm->AccessParam(6));
    epi::vec3_c br = VM_VectorToVec3(vm->AccessParam(7));

    SC_gradient_box(x, y, w, h, tl, bl, tr, br);
}

// hud.draw_image(x, y, name, [noOffset])
// if we specify noOffset then it ignores
// X and Y offsets from doom or images.ddf
//
static void HD_draw_image(coal::vm_c *vm, int argc)
{
    (void)argc;

    float       x        = *vm->AccessParam(0);
    float       y        = *vm->AccessParam(1);
    const char *name     = vm->AccessParamString(2);
    double     *noOffset = vm->AccessParam(3);

    SC_draw_image(x, y, name, noOffset ? true : false);
}

// Dasho 2022: Same as above but adds x/y texcoord scrolling
// hud.scroll_image(x, y, name, sx, sy, [noOffset])
//
static void HD_scroll_image(coal::vm_c *vm, int argc)
{
    (void)argc;

    float       x    = *vm->AccessParam(0);
    float       y    = *vm->AccessParam(1);
    const char *name = vm->AccessParamString(2);
    float       sx   = *vm->AccessParam(3);
    float       sy   = *vm->AccessParam(4);

    double *noOffset = vm->AccessParam(5);

    SC_scroll_image(x, y, name, sx, sy, noOffset ? true : false);
}

// hud.stretch_image(x, y, w, h, name, [noOffset])
// if we specify noOffset then it ignores
// X and Y offsets from doom or images.ddf
//
static void HD_stretch_image(coal::vm_c *vm, int argc)
{
    (void)argc;

    float       x        = *vm->AccessParam(0);
    float       y        = *vm->AccessParam(1);
    float       w        = *vm->AccessParam(2);
    float       h        = *vm->AccessParam(3);
    const char *name     = vm->AccessParamString(4);
    double     *noOffset = vm->AccessParam(5);

    SC_stretch_image(x, y, w, h, name, noOffset ? true : false);
}

// hud.tile_image(x, y, w, h, name, offset_x, offset_y)
//
static void HD_tile_image(coal::vm_c *vm, int argc)
{
    (void)argc;

    float       x        = *vm->AccessParam(0);
    float       y        = *vm->AccessParam(1);
    float       w        = *vm->AccessParam(2);
    float       h        = *vm->AccessParam(3);
    const char *name     = vm->AccessParamString(4);
    float       offset_x = *vm->AccessParam(5);
    float       offset_y = *vm->AccessParam(6);

    SC_tile_image(x, y, w, h, name, offset_x, offset_y);
}

// hud.draw_text(x, y, str, [size])
//
static void HD_draw_text(coal::vm_c *vm, int argc)
{
    (void)argc;

    float x = *vm->AccessParam(0);
    float y = *vm->AccessParam(1);

    const char *str = vm->AccessParamString(2);

    double *size = vm->AccessParam(3);

    SC_draw_text(x, y, str, size ? *size : 0);
}

// hud.draw_num2(x, y, len, num, [size])
//
static void HD_draw_num2(coal::vm_c *vm, int argc)
{
    (void)argc;

    float x = *vm->AccessParam(0);
    float y = *vm->AccessParam(1);

    int len = (int)*vm->AccessParam(2);
    int num = (int)*vm->AccessParam(3);

    double *size = vm->AccessParam(4);

    SC_draw_num2(x, y, len, num, size ? *size : 0);
}

// Lobo November 2021:
// hud.draw_number(x, y, len, num, align_right, [size])
//
static void HD_draw_number(coal::vm_c *vm, int argc)
{
    (void)argc;

    float x = *vm->AccessParam(0);
    float y = *vm->AccessParam(1);

    int     len         = (int)*vm->AccessParam(2);
    int     num         = (int)*vm->AccessParam(3);
    int     align_right = (int)*vm->AccessParam(4);
    double *size        = vm->AccessParam(5);

    SC_draw_number(x, y, len, num, align_right, size ? *size : 0);
}

// hud.game_paused()
//
static void HD_game_paused(coal::vm_c *vm, int argc)
{
    (void)argc;
    vm->ReturnFloat(SC_game_paused() ? 1.0f : 0.0f);
}

// hud.erraticism_active()
//
static void HD_erraticism_active(coal::vm_c *vm, int argc)
{
    (void)argc;
    vm->ReturnFloat(SC_erraticism_active() ? 1.0f : 0.0f);
}

// hud.time_stop_active()
//
static void HD_time_stop_active(coal::vm_c *vm, int argc)
{
    (void)argc;
    vm->ReturnFloat(SC_time_stop_active() ? 1 : 0);
}

// hud.render_world(x, y, w, h, [flags])
//
static void HD_render_world(coal::vm_c *vm, int argc)
{
    (void)argc;

    float x = *vm->AccessParam(0);
    float y = *vm->AccessParam(1);
    float w = *vm->AccessParam(2);
    float h = *vm->AccessParam(3);

    double *flags = vm->AccessParam(4);

    SC_render_world(x, y, w, h, flags ? (int)*flags : 0);
}

// hud.render_automap(x, y, w, h, [flags])
//
static void HD_render_automap(coal::vm_c *vm, int argc)
{
    (void)argc;

    float x = *vm->AccessParam(0);
    float y = *vm->AccessParam(1);
    float w = *vm->AccessParam(2);
    float h = *vm->AccessParam(3);

    double *flags = vm->AccessParam(4);

    SC_render_automap(x, y, w, h, flags ? (int)*flags : 0);
}

// hud.automap_color(which, color)
//
static void HD_automap_color(coal::vm_c *vm, int argc)
{
    (void)argc;

    int         which = (int)*vm->AccessParam(0);
    epi::vec3_c rgb   = VM_VectorToVec3(vm->AccessParam(1));

    SC_automap_color(which, rgb);
}

// hud.automap_option(which, value)
//
static void HD_automap_option(coal::vm_c *vm, int argc)
{
    (void)argc;

    int which = (int)*vm->AccessParam(0);
    int value = (int)*vm->AccessParam(1);

    SC_automap_option(which, value);
}

// hud.automap_zoom(value)
//
static void HD_automap_zoom(coal::vm_c *vm, int argc)
{
    (void)argc;
    float zoom = *vm->AccessParam(0);

    SC_automap_zoom(zoom);
}

// hud.automap_player_arrow(type)
//
static void HD_automap_player_arrow(coal::vm_c *vm, int argc)
{
    (void)argc;
    int arrow = (int)*vm->AccessParam(0);

    SC_automap_player_arrow(arrow);
}

// hud.set_render_who(index)
//
static void HD_set_render_who(coal::vm_c *vm, int argc)
{

    int index = (int)*vm->AccessParam(0);
    SC_set_render_who(index);
}

// hud.play_sound(name)
//
static void HD_play_sound(coal::vm_c *vm, int argc)
{
    (void)argc;
    const char *name = vm->AccessParamString(0);

    SC_play_sound(name);
}

// hud.screen_aspect()
//
static void HD_screen_aspect(coal::vm_c *vm, int argc)
{
    (void)argc;
    vm->ReturnFloat(SC_screen_aspect());
}

static void HD_get_average_color(coal::vm_c *vm, int argc)
{
    (void)argc;

    const char *name   = vm->AccessParamString(0);
    double     *from_x = vm->AccessParam(1);
    double     *to_x   = vm->AccessParam(2);
    double     *from_y = vm->AccessParam(3);
    double     *to_y   = vm->AccessParam(4);

    epi::vec3_c rgb  = SC_get_average_color(name, from_x ? *from_x : -1, to_x ? *to_x : 1000000, from_y ? *from_y : -1,
                                           to_y ? *to_y : 1000000);
    double      v[3] = {rgb.x, rgb.y, rgb.z};
    vm->ReturnVector(v);
}

static void HD_get_lightest_color(coal::vm_c *vm, int argc)
{
    (void)argc;

    const char *name   = vm->AccessParamString(0);
    double     *from_x = vm->AccessParam(1);
    double     *to_x   = vm->AccessParam(2);
    double     *from_y = vm->AccessParam(3);
    double     *to_y   = vm->AccessParam(4);

    epi::vec3_c rgb  = SC_get_lightest_color(name, from_x ? *from_x : -1, to_x ? *to_x : 1000000, from_y ? *from_y : -1,
                                            to_y ? *to_y : 1000000);
    double      v[3] = {rgb.x, rgb.y, rgb.z};
    vm->ReturnVector(v);
}

static void HD_get_darkest_color(coal::vm_c *vm, int argc)
{
    (void)argc;

    const char *name   = vm->AccessParamString(0);
    double     *from_x = vm->AccessParam(1);
    double     *to_x   = vm->AccessParam(2);
    double     *from_y = vm->AccessParam(3);
    double     *to_y   = vm->AccessParam(4);

    epi::vec3_c rgb  = SC_get_darkest_color(name, from_x ? *from_x : -1, to_x ? *to_x : 1000000, from_y ? *from_y : -1,
                                           to_y ? *to_y : 1000000);
    double      v[3] = {rgb.x, rgb.y, rgb.z};
    vm->ReturnVector(v);
}

static void HD_get_average_hue(coal::vm_c *vm, int argc)
{
    (void)argc;

    const char *name   = vm->AccessParamString(0);
    double     *from_x = vm->AccessParam(1);
    double     *to_x   = vm->AccessParam(2);
    double     *from_y = vm->AccessParam(3);
    double     *to_y   = vm->AccessParam(4);

    epi::vec3_c rgb  = SC_get_average_hue(name, from_x ? *from_x : -1, to_x ? *to_x : 1000000, from_y ? *from_y : -1,
                                         to_y ? *to_y : 1000000);
    double      v[3] = {rgb.x, rgb.y, rgb.z};
    vm->ReturnVector(v);
}

// These two aren't really needed anymore with the AverageColor rework, but keeping them in case COALHUDS in the wild
// use them - Dasho
static void HD_get_average_top_border_color(coal::vm_c *vm, int argc)
{
    (void)argc;

    const char *name = vm->AccessParamString(0);
    epi::vec3_c rgb  = SC_get_average_top_border_color(name);
    double      v[3] = {rgb.x, rgb.y, rgb.z};
    vm->ReturnVector(v);
}
static void HD_get_average_bottom_border_color(coal::vm_c *vm, int argc)
{
    (void)argc;

    const char *name = vm->AccessParamString(0);
    epi::vec3_c rgb  = SC_get_average_bottom_border_color(name);
    double      v[3] = {rgb.x, rgb.y, rgb.z};
    vm->ReturnVector(v);
}

// hud.rts_enable(tag)
//
static void HD_rts_enable(coal::vm_c *vm, int argc)
{
    (void)argc;
    SC_rts_enable(vm->AccessParamString(0));
}

// hud.rts_disable(tag)
//
static void HD_rts_disable(coal::vm_c *vm, int argc)
{
    (void)argc;
    SC_rts_disable(vm->AccessParamString(0));
}

// hud.rts_isactive(tag)
//
static void HD_rts_isactive(coal::vm_c *vm, int argc)
{
    (void)argc;

    vm->ReturnFloat(SC_rts_isactive(vm->AccessParamString(0)) ? 1 : 0);
}

// hud.get_image_width(name)
//
static void HD_get_image_width(coal::vm_c *vm, int argc)
{
    (void)argc;
    const char *name = vm->AccessParamString(0);
    vm->ReturnFloat(SC_get_image_width(name));
}

// hud.get_image_height(name)
//
static void HD_get_image_height(coal::vm_c *vm, int argc)
{
    (void)argc;
    const char *name = vm->AccessParamString(0);
    vm->ReturnFloat(SC_get_image_height(name));
}

//------------------------------------------------------------------------
// HUD Functions
//------------------------------------------------------------------------

void VM_RegisterHUD()
{
    // query functions
    ui_vm->AddNativeFunction("hud.game_mode", HD_game_mode);
    ui_vm->AddNativeFunction("hud.game_name", HD_game_name);
    ui_vm->AddNativeFunction("hud.map_name", HD_map_name);
    ui_vm->AddNativeFunction("hud.map_title", HD_map_title);
    ui_vm->AddNativeFunction("hud.map_author", HD_map_author);

    ui_vm->AddNativeFunction("hud.which_hud", HD_which_hud);
    ui_vm->AddNativeFunction("hud.check_automap", HD_check_automap);
    ui_vm->AddNativeFunction("hud.get_time", HD_get_time);

    // set-state functions
    ui_vm->AddNativeFunction("hud.coord_sys", HD_coord_sys);

    ui_vm->AddNativeFunction("hud.text_font", HD_text_font);
    ui_vm->AddNativeFunction("hud.text_color", HD_text_color);
    ui_vm->AddNativeFunction("hud.set_scale", HD_set_scale);
    ui_vm->AddNativeFunction("hud.set_alpha", HD_set_alpha);

    ui_vm->AddNativeFunction("hud.set_render_who", HD_set_render_who);
    ui_vm->AddNativeFunction("hud.automap_color", HD_automap_color);
    ui_vm->AddNativeFunction("hud.automap_option", HD_automap_option);
    ui_vm->AddNativeFunction("hud.automap_zoom", HD_automap_zoom);
    ui_vm->AddNativeFunction("hud.automap_player_arrow", HD_automap_player_arrow);

    // drawing functions
    ui_vm->AddNativeFunction("hud.solid_box", HD_solid_box);
    ui_vm->AddNativeFunction("hud.solid_line", HD_solid_line);
    ui_vm->AddNativeFunction("hud.thin_box", HD_thin_box);
    ui_vm->AddNativeFunction("hud.gradient_box", HD_gradient_box);

    ui_vm->AddNativeFunction("hud.draw_image", HD_draw_image);
    ui_vm->AddNativeFunction("hud.stretch_image", HD_stretch_image);
    ui_vm->AddNativeFunction("hud.scroll_image", HD_scroll_image);

    ui_vm->AddNativeFunction("hud.tile_image", HD_tile_image);
    ui_vm->AddNativeFunction("hud.draw_text", HD_draw_text);
    ui_vm->AddNativeFunction("hud.draw_num2", HD_draw_num2);

    ui_vm->AddNativeFunction("hud.draw_number", HD_draw_number);
    ui_vm->AddNativeFunction("hud.game_paused", HD_game_paused);
    ui_vm->AddNativeFunction("hud.erraticism_active", HD_erraticism_active);
    ui_vm->AddNativeFunction("hud.time_stop_active", HD_time_stop_active);
    ui_vm->AddNativeFunction("hud.screen_aspect", HD_screen_aspect);

    ui_vm->AddNativeFunction("hud.render_world", HD_render_world);
    ui_vm->AddNativeFunction("hud.render_automap", HD_render_automap);

    // sound functions
    ui_vm->AddNativeFunction("hud.play_sound", HD_play_sound);

    // image color functions
    ui_vm->AddNativeFunction("hud.get_average_color", HD_get_average_color);
    ui_vm->AddNativeFunction("hud.get_average_top_border_color", HD_get_average_top_border_color);
    ui_vm->AddNativeFunction("hud.get_average_bottom_border_color", HD_get_average_bottom_border_color);
    ui_vm->AddNativeFunction("hud.get_lightest_color", HD_get_lightest_color);
    ui_vm->AddNativeFunction("hud.get_darkest_color", HD_get_darkest_color);
    ui_vm->AddNativeFunction("hud.get_average_hue", HD_get_average_hue);

    ui_vm->AddNativeFunction("hud.rts_enable", HD_rts_enable);
    ui_vm->AddNativeFunction("hud.rts_disable", HD_rts_disable);
    ui_vm->AddNativeFunction("hud.rts_isactive", HD_rts_isactive);

    ui_vm->AddNativeFunction("hud.get_image_width", HD_get_image_width);
    ui_vm->AddNativeFunction("hud.get_image_height", HD_get_image_height);
}

void VM_NewGame(void)
{
    VM_CallFunction(ui_vm, "new_game");
}

void VM_LoadGame(void)
{
    // Need to set these to prevent NULL references if using any player.xxx in the load_level hook
    ui_hud_who    = players[displayplayer];
    ui_player_who = players[displayplayer];

    VM_CallFunction(ui_vm, "load_game");
}

void VM_SaveGame(void)
{
    VM_CallFunction(ui_vm, "save_game");
}

void VM_BeginLevel(void)
{
    // Need to set these to prevent NULL references if using player.xxx in the begin_level hook
    ui_hud_who    = players[displayplayer];
    ui_player_who = players[displayplayer];
    VM_CallFunction(ui_vm, "begin_level");
}

void VM_EndLevel(void)
{
    VM_CallFunction(ui_vm, "end_level");
}

void VM_RunHud(void)
{
    /*
    HUD_Reset();

    ui_hud_who    = players[displayplayer];
    ui_player_who = players[displayplayer];

    ui_hud_automap_flags[0] = 0;
    ui_hud_automap_flags[1] = 0;
    ui_hud_automap_zoom     = -1;

    VM_CallFunction(ui_vm, "draw_all");

    HUD_Reset();
    */
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
