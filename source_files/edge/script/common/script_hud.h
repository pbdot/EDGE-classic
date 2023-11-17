
#pragma once

#include "math_vector.h"

namespace script_common
{

const char *SC_game_mode();

const char *SC_game_name();

const char *SC_map_name();

const char *SC_map_title();

const char *SC_map_author();

int SC_which_hud();

bool SC_check_automap();

double SC_get_time();

void SC_text_font(const char *font_name);

void SC_text_color(const epi::vec3_c &v);

void SC_set_scale(float scale);

void SC_set_alpha(float alpha);

void SC_solid_box(float x, float y, float w, float h, const epi::vec3_c &rgb);

void SC_solid_line(float x1, float y1, float x2, float y2, const epi::vec3_c &rgb);

void SC_thin_box(float x, float y, float w, float h, const epi::vec3_c &rgb);

void SC_gradient_box(float x, float y, float w, float h, const epi::vec3_c &tl, const epi::vec3_c &bl,
                     const epi::vec3_c &tr, const epi::vec3_c &br);

void SC_draw_image(float x, float y, const char *name, bool noOffset = false);

void SC_scroll_image(float x, float y, const char *name, float sx, float sy, bool noOffset = false);

void SC_stretch_image(float x, float y, float w, float h, const char *name, bool noOffset = false);

void SC_tile_image(float x, float y, float w, float h, const char *name, float offset_x, float offset_y);

void SC_draw_text(float x, float y, const char *str, float size = 0);

void SC_draw_num2(float x, float y, int len, int num, int size = 0);

void SC_draw_number(float x, float y, int len, int num, int align_right, float size = 0.0f);

bool SC_game_paused();

bool SC_erraticism_active();

bool SC_time_stop_active();

void SC_render_world(float x, float y, float w, float h, int flags = 0);

void SC_render_automap(float x, float y, float w, float h, int flags = 0);

void SC_automap_color(int which, const epi::vec3_c &rgb);

void SC_automap_option(int which, int value);

void SC_automap_zoom(float zoom);

void SC_automap_player_arrow(int arrow);

void SC_set_render_who(int index);

void SC_play_sound(const char *name);

float SC_screen_aspect();

epi::vec3_c SC_get_average_color(const char *name, float from_x = -1, float to_x = 1000000, float from_y = -1,
                                 float to_y = 1000000);

epi::vec3_c SC_get_lightest_color(const char *name, float from_x = -1, float to_x = 1000000, float from_y = -1,
                                  float to_y = 1000000);

epi::vec3_c SC_get_darkest_color(const char *name, float from_x = -1, float to_x = 1000000, float from_y = -1,
                                 float to_y = 1000000);

epi::vec3_c SC_get_average_hue(const char *name, float from_x = -1, float to_x = 1000000, float from_y = -1,
                               float to_y = 1000000);

epi::vec3_c SC_get_average_top_border_color(const char *name = nullptr);
epi::vec3_c SC_get_average_bottom_border_color(const char *name = nullptr);

// hud.rts_enable(tag);
//
void SC_rts_enable(const char *name);

// hud.rts_disable(tag);
//
void SC_rts_disable(const char *name);
// hud.rts_isactive(tag);
//
bool SC_rts_isactive(const char *name);

// hud.get_image_width(name);
//
int SC_get_image_width(const char *name = nullptr);

// hud.get_image_height(name);
//
int SC_get_image_height(const char *name = nullptr);

} // namespace script_common

// TODO: clean these up
extern int       ui_hud_automap_flags[2];
extern float     ui_hud_automap_zoom;
extern player_t *ui_player_who;
extern player_t *ui_hud_who;

